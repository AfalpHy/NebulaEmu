# Controller
游戏的输入设备（如手柄、键盘等），输入设备内存在一个移位寄存器（8位），和一个用于保存按键状态的寄存器（8位），代表8个按钮  
A  
B  
Select  
Start  
Up  
Down  
Left  
Right  

CPU通过0x4016地址读取控制器1的数据，通过0x4017地址读取控制器2的数据，通过写0x4016地址控制读取的方式。当CPU向0x4016写入1时，输入设备不停地将保存按键状态的寄存器的数据写入到移位寄存器中，此时读取控制器将返回保存按键状态的寄存器的最低位，也就是返回A键是否被按下。当CPU向0x4016写入0时，输入设备停止将保存按键状态的寄存器的数据写入到移位寄存器中，此时读取控制器将返回移位寄存器的最低位，同时将移位寄存器的数据右移一位