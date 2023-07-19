# Traffic-Violation-Recognition-Based-on-Yolov2-with-Hi3516-H3861
利用海思的Hi3516DV300和Hi3861V100，结合yolov2实现了对斑马线道路闯红灯和骑电动不佩戴头盔的情况识别，并提供亮灯警示和语音提醒。

# ai_sample_cut
Hi3861板端代码，实现了对闯红灯、电动车不佩戴头盔违规行为的识别。
内部代码主要功能如下：
ai_infer_process ：AI处理相关
dependency ：音频播放相关函数
ext_util ：socket 相关函数以及其他一些数据处理函数
interconnection_server ：和 Pedasus wifi 通信相关
jpeg ：libjpeg 开源库
mpi ：libmpi.a 静态库
mpp ：海思媒体处理平台包含的函数相关
scenario ：子文件夹内包含重要函数：模型加载以及创建各个模块线程，模型推理以及相应后处理与各模块执行
smp ：代码入口，配置系统初始化相关
third_party ： opencv 相关

# oc_demo
Hi3516板端代码，负责与Hi3861通信，以及控制电路寄存器实现对警示灯和红绿灯的控制。
