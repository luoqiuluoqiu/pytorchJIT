#include <QCoreApplication>
#include<QDebug>
#include <script.h>
#include <torch.h>

#include <opencv.hpp>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
    // Deserialize the ScriptModule from a file using torch::jit::load().
     std::shared_ptr<torch::jit::script::Module> module = torch::jit::load("D:/model.pt");
     assert(module != nullptr);
     std::cout << "load model ok\n";

     // Create a vector of inputs.
     std::vector<torch::jit::IValue> inputs;
     inputs.push_back(torch::rand({1, 3, 224, 224}));

     // evalute time
     double t = (double)cv::getTickCount();
     module->forward(inputs).toTensor();
     t = (double)cv::getTickCount() - t;
     printf("execution time = %gs\n", t / cv::getTickFrequency());
     inputs.pop_back();

     // load image with opencv and transform
     cv::Mat image;
     image = cv::imread("D:/src.png", 1);
     cv::cvtColor(image, image, cv::COLOR_BGR2RGB);
     cv::Mat img_float;
     image.convertTo(img_float, CV_32F, 1.0/255);
     cv::resize(img_float, img_float, cv::Size(224, 224));
     //std::cout << img_float.at<cv::Vec3f>(56,34)[1] << std::endl;
     auto img_tensor = torch::CPU(torch::kFloat32).tensorFromBlob(img_float.data, {1, 224, 224, 3});
     img_tensor = img_tensor.permute({0,3,1,2});
     img_tensor[0][0] = img_tensor[0][0].sub_(0.485).div_(0.229);
     img_tensor[0][1] = img_tensor[0][1].sub_(0.456).div_(0.224);
     img_tensor[0][2] = img_tensor[0][2].sub_(0.406).div_(0.225);
     auto img_var = torch::autograd::make_variable(img_tensor, false);
     inputs.push_back(img_var);

     // Execute the model and turn its output into a tensor.
     torch::Tensor out_tensor = module->forward(inputs).toTensor();
     std::cout << out_tensor.slice(/*dim=*/1, /*start=*/0, /*end=*/10) << '\n';

     // Load labels
     std::string label_file = "C:/libtorch/synset_words.txt";
     std::ifstream rf(label_file.c_str());
     CHECK(rf) << "Unable to open labels file " << label_file;
     std::string line;
     std::vector<std::string> labels;
     while (std::getline(rf, line)){
       labels.push_back(line);
       //std::cout<<line<<std::endl;
     }
     // print predicted top-5 labels
     std::tuple<torch::Tensor,torch::Tensor> result = out_tensor.sort(-1, true);
     torch::Tensor top_scores = std::get<0>(result)[0];
     torch::Tensor top_idxs = std::get<1>(result)[0].toType(torch::kInt32);

     auto top_scores_a = top_scores.accessor<float,1>();
     auto top_idxs_a = top_idxs.accessor<int,1>();
   //std::cout<<"$$$$$$$$$$$$$$$$"<<std::endl;
     for (int i = 0; i < 5; ++i) {
       int idx = top_idxs_a[i];
       std::cout<<idx<<std::endl;
       std::cout << "top-" << i+1 << " label: ";
       std::cout<<"$$$$$$$$$$$$$$$$:"<<labels[idx]<<std::endl;
       std::cout << "score: " << top_scores_a[i] << std::endl;
     }
    return a.exec();
}
