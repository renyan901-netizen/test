
// //云台+追踪
// #include "ti_msp_dl_config.h"
// #include "oled_hardware_i2c.h"
// #include "k230.h"
// #include "gimbal.h"
// #include "pid.h"

// // 专门为本次测试独立创建的 PID 结构体
// PID_TypeDef pid_test_x;
// PID_TypeDef pid_test_y;

// int main(void) {
//     // 1. 初始化底层硬件
//     SYSCFG_DL_init();

//     // 2. 初始化中断 (K230 串口接收必备)
//     NVIC_EnableIRQ(UART_K230_INST_INT_IRQN);

//     // 3. 初始化外设
//     OLED_Init();
//     OLED_Clear();
//     OLED_ShowString(0, 0, (uint8_t*)"[Gimbal Tracker]", 16);

//     // 🚨 极其重要：在此刻按下电源开关前，请确保摄像头已经手动掰到正中间！
//     Gimbal_Init(); 

//     // =========================================================
//     // 🚀 追踪不及时修复：调大 KP！
//     // 之前是 5.0，现在加到 8.0，让云台爆发力更强，跟手更紧！
//     // =========================================================
//     // PID_Init(&pid_test_x, 8.0f, 0.0f, 1.0f);
//     // PID_Init(&pid_test_y, 8.0f, 0.0f, 1.0f);
//     // 第三个参数原来是 0.0f，现在给它加一点点积分 (比如 0.2 到 0.5)
//     // 修改为：降低 KP，彻底清零 KI，大幅提高 KD
// PID_Init(&pid_test_x, 9.0f, 0.0f, 1.5f);
// PID_Init(&pid_test_y, 9.0f, 0.0f, 1.5f);
//     uint16_t loop_cnt = 0;
//     uint16_t target_lost_timeout = 0;

//     while (1) {
//         if (k230_update_flag == 1) {
//             k230_update_flag = 0;
            
//             if (k230_target_x < 5 && k230_target_y < 5) {
//                 if (target_lost_timeout < 100) target_lost_timeout++; 
//             } 
//             else {
//                 target_lost_timeout = 0; 

//                static float smooth_x = 320.0f;
//                 static float smooth_y = 240.0f;
                
//                 // 🚨 撕掉滤镜！从 0.5 暴增到 0.85！
//                 // 允许视觉模块 85% 的实时数据直接砸进系统，延迟瞬间消失！
//                 float alpha = 0.85f; 
//                 smooth_x = (alpha * k230_target_x) + ((1.0f - alpha) * smooth_x);
//                 smooth_y = (alpha * k230_target_y) + ((1.0f - alpha) * smooth_y);

//                 float LASER_OFFSET_X = 30.0f; 
//                 float LASER_OFFSET_Y = 0.0f;   

//                 float offset_x = (320.0f - smooth_x) + LASER_OFFSET_X; 
//                 float offset_y = (240.0f - smooth_y) + LASER_OFFSET_Y; 

//                 // 🚨 恢复你原本的 8.0 灵敏死区
//                 if (offset_x > -8.0f && offset_x < 8.0f) offset_x = 0;
//                 if (offset_y > -8.0f && offset_y < 8.0f) offset_y = 0;

//                 // 死区过滤 (保持不变)
//                 if (offset_x > -8.0f && offset_x < 8.0f) offset_x = 0;
//                 if (offset_y > -8.0f && offset_y < 8.0f) offset_y = 0;

//                 // =========================================================
//                 // 🚀 1. 云台 X 轴专属“分段 PID”：专治底盘急转弯！
//                 // =========================================================
//                 float abs_offset_x = (offset_x > 0) ? offset_x : -offset_x;
                
//                 if (abs_offset_x > 40.0f) {
//                     // 【甩枪区】当底盘急转，靶心瞬间偏离中心超过 40 个像素时！
//                     // 🚨 开启“锁头外挂”：极大提升 KP，强行把云台甩过去！
//                     pid_test_x.Kp = 16.0f; 
//                     pid_test_x.Kd = 4.0f;  // 配合高 P 增加一点刹车防过冲
//                 } else {
//                     // 【狙击区】靶心回到了中心附近，底盘在走直线
//                     // 恢复极其稳定的神仙参数，死死咬住红心
//                     pid_test_x.Kp = 9.0f;
//                     pid_test_x.Kd = 2.5f;
//                 }

//                 // Y 轴不需要分段，因为底盘转弯基本不影响垂直高度
//                 pid_test_y.Kp = 9.0f;
//                 pid_test_y.Kd = 2.5f;

//                 // 计算 PID 速度
//                 float speed_x = PID_Calc(&pid_test_x, offset_x);
//                 float speed_y = PID_Calc(&pid_test_y, offset_y);

//                 // =========================================================
//                 // 🚨 2. 解除云台速度封印！
//                 // 把 150 放宽到 600（或者你舵机/电机 PWM 允许的安全最大值）
//                 // 给云台瞬间爆发出极高角速度的物理权限！
//                 // =========================================================
//                 float max_gimbal_speed = 600.0f; // 彻底释放野兽！
                
//                 if(speed_x > max_gimbal_speed)  speed_x = max_gimbal_speed;
//                 if(speed_x < -max_gimbal_speed) speed_x = -max_gimbal_speed;
//                 if(speed_y > max_gimbal_speed)  speed_y = max_gimbal_speed;
//                 if(speed_y < -max_gimbal_speed) speed_y = -max_gimbal_speed;

//                 // 开启双轴联动
//                 Gimbal_SetSpeed_X(speed_x);
//                 Gimbal_SetSpeed_Y(speed_y);
//             }

//             loop_cnt++;
//             if (loop_cnt > 10) {
//                 loop_cnt = 0;
//                 // ... OLED 刷新逻辑保持不变
//             }
//         } 
//         else {
//             if (target_lost_timeout < 100) target_lost_timeout++;
//         }
        
//         // 【安全机制】目标丢失保护 (超时彻底刹死)
//         if (target_lost_timeout >= 100) {
//             Gimbal_SetSpeed_X(0);
//             Gimbal_SetSpeed_Y(0);
//             pid_test_x.integral = 0; 
//             pid_test_y.integral = 0;
//         }

//         delay_cycles(160000); 
//     }
// }



// #include "ti_msp_dl_config.h"
// #include "track.h"
// #include "motor.h"
// #include "oled_hardware_i2c.h"
// #include "pid.h"
// #include "key.h"  

// void delay_seconds(uint32_t s);

// volatile uint8_t car_running = 0;  
// extern volatile int32_t left_pulse_count;
// extern volatile int32_t right_pulse_count;

// PID_TypeDef pid_left;
// PID_TypeDef pid_right;

// // ==========================================
// // 🚗 底层重车 PID 参数 (速度环纯 PI，无抖动！)
// // ==========================================
// #define SPEED_KP 1.0f  
// #define SPEED_KI 0.2f  
// #define SPEED_KD 0.0f  

// #define TRACK_KP 2.8f  
// #define TRACK_KD 20.0f 

// // ==========================================
// // 🌟 赛场交互 UI 与全局控制变量
// // ==========================================
// float STRAIGHT_SPEED = 3.0f;   
// float CURVE_SPEED    = 1.5f;   

// uint8_t  target_laps = 1;      
// uint8_t  speed_mode  = 0;      
// uint8_t  start_flag  = 0;      

// uint16_t corner_count   = 0;   
// uint16_t debounce_timer = 0;   
// uint16_t oled_refresh_cnt = 0; 
// #define CORNERS_PER_LAP 4      

// // ==========================================
// // 🚨 终极：全直道动态相减补偿变量
// // ==========================================
// uint32_t t_first = 0;      // 记录 起点->首弯 的时间
// uint32_t t_full = 0;       // 记录 首弯->第二弯 (完整直道) 的时间
// uint32_t homing_ticks = 0; // 冲刺计时
// uint8_t  track_state = 0;  // 0:待机, 1:测首段, 2:测完整直道, 3:途中巡航, 4:归位冲刺

// float current_base_speed = 0.0f; 
// float last_valid_pos = 4.5f; 
// float last_pos_error = 0.0f; 

// uint32_t white_area_cnt = 0;
// #define WHITE_TIMEOUT 60 

// static uint8_t curve_mode = 0; 

// float Get_Track_Position(void) {
//     float sum = 0;
//     int active_cnt = 0;
//     for(int i = 0; i < 8; i++) {
//         if(IR_Data[i]) { 
//             sum += (i + 1.0f);
//             active_cnt++;
//         }
//     }
//     if(active_cnt == 0) return 0.0f; 
//     return sum / active_cnt;
// }

// int main(void) {
//     SYSCFG_DL_init();
    
//     PID_Init(&pid_left, SPEED_KP, SPEED_KI, SPEED_KD);
//     PID_Init(&pid_right, SPEED_KP, SPEED_KI, SPEED_KD);
    
//     NVIC_ClearPendingIRQ(GPIOB_INT_IRQn);
//     NVIC_EnableIRQ(GPIOB_INT_IRQn);       
//     NVIC_ClearPendingIRQ(TIMER_PID_INST_INT_IRQN);
//     NVIC_EnableIRQ(TIMER_PID_INST_INT_IRQN); 
    
//     Motor_Init(); 
//     DL_TimerG_startCounter(TIMER_PID_INST);
//     OLED_Init();
//     Track_Init(); 

//     delay_seconds(1);
//     left_pulse_count = 0;
//     right_pulse_count = 0;

//     OLED_Clear();
    
//     while (1) {
//         // ========================================================
//         // 🎮 1. 按键扫描与菜单逻辑 
//         // ========================================================
//         uint8_t key_val = Key_Scan(0);
        
//         if(key_val == KEY1_PRES) {
//             target_laps++;
//             if(target_laps > 5) target_laps = 0; // 支持 0.5 圈
//         }
//         else if(key_val == KEY2_PRES) {
//             if (speed_mode == 0) {
//                 speed_mode = 1;
//                 STRAIGHT_SPEED = 5.0f;  
//                 CURVE_SPEED    = 2.5f;
//             } else {
//                 speed_mode = 0;
//                 STRAIGHT_SPEED = 3.0f;  
//                 CURVE_SPEED    = 1.5f;
//             }
//         }
//         else if(key_val == KEY3_PRES) {
//             if(start_flag == 0) {
//                 start_flag = 1;      
//                 car_running = 1;
//                 corner_count = 0;    
//                 debounce_timer = 0;  
                
//                 // 触发全轨道算法：清零所有测距时间
//                 track_state = 1; 
//                 t_first = 0;
//                 t_full = 0;
//                 homing_ticks = 0;

//                 current_base_speed = 0.5f; 
//                 OLED_ShowString(48, 4, (uint8_t *)"RUN... ", 16);
//             } else {
//                 start_flag = 0;      
//                 car_running = 0;
//                 track_state = 0;
//                 OLED_ShowString(48, 4, (uint8_t *)"STOP   ", 16);
//             }
//         }

//         // ========================================================
//         // 📺 2. 刷新 UI
//         // ========================================================
//         if (start_flag == 0) { 
//             oled_refresh_cnt++;
//             if (oled_refresh_cnt >= 20) { 
//                 oled_refresh_cnt = 0;

//                 OLED_ShowString(0, 0, (uint8_t *)"Laps: ", 16);
//                 if (target_laps == 0) {
//                     OLED_ShowString(48, 0, (uint8_t *)"0.5/5", 16);
//                 } else {
//                     OLED_ShowNum(48, 0, target_laps, 1, 16);
//                     OLED_ShowString(56, 0, (uint8_t *)".0/5", 16); 
//                 }

//                 OLED_ShowString(0, 2, (uint8_t *)"Mode: ", 16);
//                 if(speed_mode == 0) OLED_ShowString(48, 2, (uint8_t *)"SAFE  ", 16);
//                 else OLED_ShowString(48, 2, (uint8_t *)"RACING", 16);

//                 OLED_ShowString(0, 4, (uint8_t *)"Stat: ", 16);
//                 OLED_ShowString(48, 4, (uint8_t *)"READY  ", 16);

//                 OLED_ShowString(0, 6, (uint8_t *)"Corn: ", 16);
//                 OLED_ShowNum(48, 6, corner_count, 2, 16);
//             }
//         }

//         // ========================================================
//         // 🚀 3. 循迹与全轨道动态补偿控制
//         // ========================================================
//         if (start_flag == 1) {
            
//             // 状态机测距
//             if (track_state == 1) t_first++;
//             else if (track_state == 2) t_full++;
//             else if (track_state == 4) homing_ticks++;

//             ReadEightIR();
//             int black_cnt = 0;
//             for(int i=0; i<8; i++) if(IR_Data[i]) black_cnt++;
            
//             float current_pos = Get_Track_Position();

//             if (debounce_timer > 0) debounce_timer--;

//             if (black_cnt >= 4 || (IR_Data[0] == 1 && IR_Data[1] == 1 && IR_Data[2] == 1)) { 
//                 if (debounce_timer == 0) {
//                     corner_count++;
//                     debounce_timer = 100; 
                    
//                     if (track_state == 1) track_state = 2;       
//                     else if (track_state == 2) track_state = 3;  
                    
//                     uint16_t target_corners = (target_laps == 0) ? 2 : (target_laps * CORNERS_PER_LAP);

//                     if (corner_count >= target_corners) {
//                         track_state = 4; 
//                     }
//                 }
//             }

//             // --- 🏁 终点万能补偿刹车判定 ---
//             int32_t target_ticks = (int32_t)t_full - (int32_t)t_first - 10;
            
//             if (target_ticks < 0) target_ticks = 0; 
            
//             if (track_state == 4 && homing_ticks >= target_ticks) {
//                 start_flag = 0;            
//                 car_running = 0;           
//                 track_state = 0;
//                 current_base_speed = 0.0f; 
//                 pid_left.target  = 0;
//                 pid_right.target = 0;
//                 OLED_ShowString(48, 4, (uint8_t *)"DONE!  ", 16);
//             }
//             else {
//                 if(car_running == 0) {
//                     car_running = 1;
//                     current_base_speed = 0.5f; 
//                 }
                
//                 if(current_pos == 0.0f) {
//                     white_area_cnt++;
//                     if(white_area_cnt > WHITE_TIMEOUT) {
//                         car_running = 0; start_flag = 0; track_state = 0;
//                         current_base_speed = 0.0f; 
//                         OLED_ShowString(48, 4, (uint8_t *)"OUT!   ", 16); 
//                     }
//                     current_pos = last_valid_pos; 
//                 } else {
//                     white_area_cnt = 0;
//                     last_valid_pos = current_pos; 
//                 }

//                 // ========================================================
//                 // 🌊 软件液压助力：误差一阶低通滤波
//                 // ========================================================
//                 static float smoothed_error = 0.0f;
//                 float raw_error = current_pos - 4.5f; 
//                 smoothed_error = 0.6f * raw_error + 0.4f * smoothed_error; 
                
//                 float error = smoothed_error; 
//                 float abs_error = (error > 0) ? error : -error;

//                 if (IR_Data[0] || IR_Data[1] || IR_Data[6] || IR_Data[7] || abs_error > 1.2f) {
//                     curve_mode = 1; 
//                 } else if (!IR_Data[0] && !IR_Data[1] && !IR_Data[6] && !IR_Data[7] && abs_error < 0.5f) {
//                     curve_mode = 0;
//                 }

//                 float target_speed = curve_mode ? CURVE_SPEED : STRAIGHT_SPEED;

//                 // ========================================================
//                 // 🚀 1. 动态刹车系统：专治高速“冲过头”
//                 // ========================================================
//                 if (current_base_speed < target_speed) {
//                     if (current_base_speed < 1.2f) current_base_speed = 1.2f; // 破静摩擦起步
//                     // 加速阶段：狂暴模式提速更快，SAFE模式平稳
//                     current_base_speed += (speed_mode == 1) ? 0.08f : 0.04f;  
//                     if (current_base_speed > target_speed) current_base_speed = target_speed;
//                 } else if (current_base_speed > target_speed) {
//                     // 🚨 终极重刹：狂暴模式下，一旦发现弯道，瞬间释放 0.40 的巨大刹车力！
//                     // 这相当于把刹车力度提升了 8 倍，强行把车速在飞出赛道前拽下来！
//                     current_base_speed -= (speed_mode == 1) ? 0.40f : 0.08f;  
//                     if (current_base_speed < target_speed) current_base_speed = target_speed;
//                 }

//                 // ========================================================
//                 // 🚑 2. 分段 PID 保持你的完美参数不变
//                 // ========================================================
//                 float error_d = error - last_pos_error;
//                 last_pos_error = error;
                
//                 float speed_adjust = 0.0f;
//                 if (abs_error <= 0.6f) {
//                     speed_adjust = (error * (TRACK_KP * 0.4f)) + (error_d * (TRACK_KD * 0.2f)); 
//                 } 
//                 else if (abs_error <= 1.5f) {
//                     speed_adjust = (error * (TRACK_KP * 0.7f)) + (error_d * (TRACK_KD * 0.5f));
//                 }
//                 else {
//                     speed_adjust = (error * TRACK_KP) + (error_d * TRACK_KD);
//                 }
                
//                 // ========================================================
//                 // 🛡️ 3. 解除狂暴模式的“转向封印”
//                 // ========================================================
//                 float max_adjust = 0.0f;
//                 if (curve_mode) {
//                     // 🚨 狂暴模式下，把弯道差速极限界限放宽到 4.5！
//                     // 此时外侧轮极速飙到 7.0，内侧轮猛烈倒转，强行把重车“甩”过直角弯！
//                     max_adjust = (speed_mode == 1) ? 4.5f : 2.8f; 
//                 } else {
//                     max_adjust = 2.5f; // 直道依然死死锁住，绝不允许摇头
//                 }
                
//                 if(speed_adjust > max_adjust) speed_adjust = max_adjust;
//                 if(speed_adjust < -max_adjust) speed_adjust = -max_adjust;

//                 pid_left.target  = current_base_speed + speed_adjust;
//                 pid_right.target = current_base_speed - speed_adjust;
//             }
//         } 
//         else {
//             car_running = 0;
//             current_base_speed = 0.0f;
//             pid_left.target = 0;
//             pid_right.target = 0;
//             Motor_SetSpeed(0, 0); 
//         }

//         delay_cycles(160000); 
//     }
// }

// void delay_seconds(uint32_t s) {
//     for(uint32_t i = 0; i < s; i++) delay_cycles(32000000); 
// }

// // ========================================================
// // 🌟 底层执行：定时器 PID 中断
// // ========================================================
// void TIMER_PID_INST_IRQHandler(void)
// {
//     switch (DL_TimerG_getPendingInterrupt(TIMER_PID_INST)) {
//         case DL_TIMER_IIDX_ZERO:
//         {   
//             float raw_speed_L = (float)left_pulse_count;
//             float raw_speed_R = (float)right_pulse_count;
//             left_pulse_count = 0;
//             right_pulse_count = 0;

//             static uint16_t startup_cnt = 0;
//             static float filtered_speed_L = 0;
//             static float filtered_speed_R = 0;

//             if (car_running == 0) {
//                 Motor_SetSpeed(0, 0);
//                 pid_left.integral = 0;
//                 pid_right.integral = 0;
//                 startup_cnt = 0; 
                
//                 filtered_speed_L = 0;
//                 filtered_speed_R = 0;
//             } else {
//                 if(startup_cnt < 100) startup_cnt++;

//                 filtered_speed_L = 0.3f * raw_speed_L + 0.7f * filtered_speed_L;
//                 filtered_speed_R = 0.3f * raw_speed_R + 0.7f * filtered_speed_R;

//                 float pid_adj_L = PID_Calc(&pid_left, filtered_speed_L);
//                 float pid_adj_R = PID_Calc(&pid_right, filtered_speed_R);

//                 float base_pwm_L = 0;
//                 float base_pwm_R = 0;

//                 if (pid_left.target > 0) {
//                     base_pwm_L = 28.0f + pid_left.target * 2.0f; 
//                 } else {
//                     base_pwm_L = pid_left.target * 6.0f; 
//                 }

//                 if (pid_right.target > 0) {
//                     base_pwm_R = 45.0f + pid_right.target * 2.0f; 
//                 } else {
//                     base_pwm_R = pid_right.target * 6.0f; 
//                 }
                
//                 // if (startup_cnt > 0 && startup_cnt <= 30) {
//                 //     float fade_ratio = (30.0f - startup_cnt) / 30.0f;
//                 //     base_pwm_R += 26.0f * fade_ratio;  
//                 //     base_pwm_L -= 2.0f * fade_ratio;   
                    
//                 //     pid_left.integral = 0;
//                 //     pid_right.integral = 170.0f * (1.0f - fade_ratio); 
//                 // }
                
//                 float pwm_out_L = base_pwm_L + pid_adj_L;
//                 float pwm_out_R = base_pwm_R + pid_adj_R;

//                 if(pwm_out_L < 8.0f && pid_left.target > 0) pwm_out_L = 8.0f;
//                 if(pwm_out_R < 8.0f && pid_right.target > 0) pwm_out_R = 8.0f;

//                 Motor_SetSpeed((int8_t)pwm_out_L, (int8_t)pwm_out_R);
//             }
//             break;
//         }
//         default: break;
//     }
// }



#include "ti_msp_dl_config.h"
#include "oled_hardware_i2c.h"
#include "key.h"
#include "track.h"
#include "motor.h"
#include "gimbal.h"
#include "k230.h"
#include "ctrl.h" 

void delay_seconds(uint32_t s) {
    for(uint32_t i = 0; i < s; i++) delay_cycles(32000000); 
}

int main(void) {
    // 1. 底层硬件与中断初始化
    SYSCFG_DL_init();
    NVIC_ClearPendingIRQ(GPIOB_INT_IRQn);
    NVIC_EnableIRQ(GPIOB_INT_IRQn);       
    NVIC_ClearPendingIRQ(TIMER_PID_INST_INT_IRQN);
    NVIC_EnableIRQ(TIMER_PID_INST_INT_IRQN); 
    NVIC_EnableIRQ(UART_K230_INST_INT_IRQN); 

    // 2. 模块初始化
    Motor_Init(); 
    Gimbal_Init();
    Track_Init(); 
    OLED_Init();

    // 3. 核心控制大脑初始化
    Ctrl_Init(); 
    DL_TimerG_startCounter(TIMER_PID_INST);

    delay_seconds(1);
    OLED_Clear();

    uint16_t oled_refresh_cnt = 0;

    while (1) {
        // ===========================================
        // [模块 1] 扫描按键并送入控制大脑
        // ===========================================
        // 🚨 确保你底层的 Key_Scan 已经支持了 K4 键值返回
        uint8_t key_val = Key_Scan(0);
        Ctrl_Key_Process(key_val);

        // ===========================================
        // [模块 2] UI 战术仪表盘刷新系统
        // ===========================================
        if (start_flag == 0) { 
            oled_refresh_cnt++;
            if (oled_refresh_cnt >= 20) { 
                oled_refresh_cnt = 0;

                // 第一行：显示圈数
                OLED_ShowString(0, 0, (uint8_t *)"Laps: ", 16);
                if (target_laps == 0) OLED_ShowString(48, 0, (uint8_t *)"0.5/5  ", 16);
                else {
                    OLED_ShowNum(48, 0, target_laps, 1, 16);
                    OLED_ShowString(56, 0, (uint8_t *)".0/5  ", 16); 
                }

                // 第二行与第三行：根据系统模式切换显示风格
                if (sys_mode == 0) {
                    // NORM 模式 UI：
                    OLED_ShowString(0, 2, (uint8_t *)"Sys : NORM    ", 16);
                    OLED_ShowString(0, 4, (uint8_t *)"Spd : ", 16);
                    if(speed_mode == 0) OLED_ShowString(48, 4, (uint8_t *)"SAFE  ", 16);
                    else OLED_ShowString(48, 4, (uint8_t *)"RACING", 16);
                } else {
                    // COMP 综合模式 UI：
                    OLED_ShowString(0, 2, (uint8_t *)"Sys : COMP    ", 16);
                    OLED_ShowString(0, 4, (uint8_t *)"Trk : ", 16);
                    // 清楚地显示底盘是移动还是锁死
                    if(track_enable == 1) OLED_ShowString(48, 4, (uint8_t *)"ON (MOVE)", 16);
                    else OLED_ShowString(48, 4, (uint8_t *)"OFF(STOP)", 16);
                }

                // 第四行：就绪状态
                OLED_ShowString(0, 6, (uint8_t *)"Stat: ", 16);
                OLED_ShowString(48, 6, (uint8_t *)"READY  ", 16);
            }
        } else {
            // Running: OLED completely disabled to free I2C bus
        }

        // ===========================================
        // [模块 3] 执行双线核心任务
        // ===========================================
        Ctrl_Chassis_Process();  // 底盘任务
        Ctrl_Gimbal_Process();   // 云台任务

        delay_cycles(160000); 
    }
}

// ========================================================
// 底层定时器中断 (转交控制权给大脑)
// ========================================================
void TIMER_PID_INST_IRQHandler(void) {
    switch (DL_TimerG_getPendingInterrupt(TIMER_PID_INST)) {
        case DL_TIMER_IIDX_ZERO:
            Ctrl_Motor_IRQ_Process(); 
            break;
        default: break;
    }
}



// //test
// #include "ti_msp_dl_config.h"
// #include "track.h"
// #include "motor.h"
// #include "oled_hardware_i2c.h"
// #include "pid.h"
// #include "key.h"  

// void delay_seconds(uint32_t s);

// volatile uint8_t car_running = 0;  
// extern volatile int32_t left_pulse_count;
// extern volatile int32_t right_pulse_count;

// PID_TypeDef pid_left;
// PID_TypeDef pid_right;

// // ==========================================
// // 🚗 底层重车 PID 参数 (速度环纯 PI)
// // ==========================================
// #define SPEED_KP 1.0f  
// #define SPEED_KI 0.2f  
// #define SPEED_KD 0.0f  

// #define TRACK_KP 2.8f  
// #define TRACK_KD 20.0f 

// // ==========================================
// // 🌟 赛场交互 UI 与全局控制变量
// // ==========================================
// float STRAIGHT_SPEED = 3.0f;   
// float CURVE_SPEED    = 1.5f;   

// uint8_t  target_laps = 1;      
// uint8_t  speed_mode  = 0;      
// uint8_t  start_flag  = 0;      

// uint16_t corner_count   = 0;   
// uint16_t debounce_timer = 0;   
// uint16_t oled_refresh_cnt = 0; 
// #define CORNERS_PER_LAP 4      

// uint32_t t_first = 0;      
// uint32_t t_full = 0;       
// uint32_t homing_ticks = 0; 
// uint8_t  track_state = 0;  

// float current_base_speed = 0.0f; 
// float last_valid_pos = 4.5f; 
// float last_pos_error = 0.0f; 

// uint32_t white_area_cnt = 0;
// #define WHITE_TIMEOUT 60 

// static uint8_t curve_mode = 0; 

// float Get_Track_Position(void) {
//     float sum = 0;
//     int active_cnt = 0;
//     for(int i = 0; i < 8; i++) {
//         if(IR_Data[i]) { 
//             sum += (i + 1.0f);
//             active_cnt++;
//         }
//     }
//     if(active_cnt == 0) return 0.0f; 
//     return sum / active_cnt;
// }

// int main(void) {
//     SYSCFG_DL_init();
    
//     PID_Init(&pid_left, SPEED_KP, SPEED_KI, SPEED_KD);
//     PID_Init(&pid_right, SPEED_KP, SPEED_KI, SPEED_KD);
    
//     NVIC_ClearPendingIRQ(GPIOB_INT_IRQn);
//     NVIC_EnableIRQ(GPIOB_INT_IRQn);       
//     NVIC_ClearPendingIRQ(TIMER_PID_INST_INT_IRQN);
//     NVIC_EnableIRQ(TIMER_PID_INST_INT_IRQN); 
    
//     Motor_Init(); 
//     DL_TimerG_startCounter(TIMER_PID_INST);
//     OLED_Init();
//     Track_Init(); 

//     delay_seconds(1);
//     left_pulse_count = 0;
//     right_pulse_count = 0;

//     OLED_Clear();
    
//     while (1) {
//         uint8_t key_val = Key_Scan(0);
        
//         if(key_val == KEY1_PRES) {
//             target_laps++;
//             if(target_laps > 5) target_laps = 0; 
//         }
//         else if(key_val == KEY2_PRES) {
//             if (speed_mode == 0) {
//                 speed_mode = 1;
//                 STRAIGHT_SPEED = 5.0f;  
//                 CURVE_SPEED    = 2.5f;
//             } else {
//                 speed_mode = 0;
//                 STRAIGHT_SPEED = 3.0f;  
//                 CURVE_SPEED    = 1.5f;
//             }
//         }
//         else if(key_val == KEY3_PRES) {
//             if(start_flag == 0) {
//                 start_flag = 1;      
//                 car_running = 1;
//                 corner_count = 0;    
//                 debounce_timer = 0;  
                
//                 track_state = 1; 
//                 t_first = 0;
//                 t_full = 0;
//                 homing_ticks = 0;

//                 current_base_speed = 0.5f; 
//                 OLED_ShowString(48, 4, (uint8_t *)"RUN... ", 16);
//             } else {
//                 start_flag = 0;      
//                 car_running = 0;
//                 track_state = 0;
//                 OLED_ShowString(48, 4, (uint8_t *)"STOP   ", 16);
//             }
//         }

//         // ========================================================
//         // 📺 UI 刷新
//         // ========================================================
//         if (start_flag == 0) { 
//             oled_refresh_cnt++;
//             if (oled_refresh_cnt >= 20) { 
//                 oled_refresh_cnt = 0;
//                 OLED_ShowString(0, 0, (uint8_t *)"Laps: ", 16);
//                 if (target_laps == 0) OLED_ShowString(48, 0, (uint8_t *)"0.5/5", 16);
//                 else {
//                     OLED_ShowNum(48, 0, target_laps, 1, 16);
//                     OLED_ShowString(56, 0, (uint8_t *)".0/5", 16); 
//                 }

//                 OLED_ShowString(0, 2, (uint8_t *)"Mode: ", 16);
//                 if(speed_mode == 0) OLED_ShowString(48, 2, (uint8_t *)"SAFE  ", 16);
//                 else OLED_ShowString(48, 2, (uint8_t *)"RACING", 16);

//                 OLED_ShowString(0, 4, (uint8_t *)"Stat: ", 16);
//                 OLED_ShowString(48, 4, (uint8_t *)"READY  ", 16);

//                 OLED_ShowString(0, 6, (uint8_t *)"Corn: ", 16);
//                 OLED_ShowNum(48, 6, corner_count, 2, 16);
//             }
//         }

//         // ========================================================
//         // 🚀 循迹主逻辑
//         // ========================================================
//         if (start_flag == 1) {
            
//             if (track_state == 1) t_first++;
//             else if (track_state == 2) t_full++;
//             else if (track_state == 4) homing_ticks++;

//             ReadEightIR();
//             int black_cnt = 0;
//             for(int i=0; i<8; i++) if(IR_Data[i]) black_cnt++;
            
//             float current_pos = Get_Track_Position();

//             if (debounce_timer > 0) debounce_timer--;

//             if (black_cnt >= 4 || (IR_Data[0] == 1 && IR_Data[1] == 1 && IR_Data[2] == 1)) { 
//                 if (debounce_timer == 0) {
//                     corner_count++;
//                     debounce_timer = 250; 
                    
//                     if (track_state == 1) track_state = 2;       
//                     else if (track_state == 2) track_state = 3;  
                    
//                     uint16_t target_corners = (target_laps == 0) ? 2 : (target_laps * CORNERS_PER_LAP);

//                     if (corner_count >= target_corners) {
//                         track_state = 4; 
//                     }
//                 }
//             }

//             int32_t target_ticks = (int32_t)t_full - (int32_t)t_first - 10;
//             if (target_ticks < 0) target_ticks = 0; 
            
//             if (track_state == 4 && homing_ticks >= target_ticks) {
//                 start_flag = 0;            
//                 car_running = 0;           
//                 track_state = 0;
//                 current_base_speed = 0.0f; 
//                 pid_left.target  = 0;
//                 pid_right.target = 0;
//                 OLED_ShowString(48, 4, (uint8_t *)"DONE!  ", 16);
//             }
//             else {
//                 if(car_running == 0) {
//                     car_running = 1;
//                     current_base_speed = 0.5f; 
//                 }
                
//                 if(current_pos == 0.0f) {
//                     white_area_cnt++;
//                     if(white_area_cnt > WHITE_TIMEOUT) {
//                         car_running = 0; start_flag = 0; track_state = 0;
//                         current_base_speed = 0.0f; 
//                         OLED_ShowString(48, 4, (uint8_t *)"OUT!   ", 16); 
//                     }
//                     current_pos = last_valid_pos; 
//                 } else {
//                     white_area_cnt = 0;
//                     last_valid_pos = current_pos; 
//                 }

//                 static float smoothed_error = 0.0f;
//                 float raw_error = current_pos - 4.5f; 
//                 smoothed_error = 0.6f * raw_error + 0.4f * smoothed_error; 
                
//                 float error = smoothed_error; 
//                 float abs_error = (error > 0) ? error : -error;

//                 // ========================================================
//                 // 🚨 修复过弯顿挫：引入弯道保持定时器 (防误判掉线)
//                 // ========================================================
//                 static uint16_t curve_hold_timer = 0;
//                // 删掉了该死的 abs_error > 1.2f！
//                 // 只有当最外侧的传感器（0, 1 或 6, 7）实打实地踩到黑线，才承认是真弯道！
//                 if (IR_Data[0] || IR_Data[1] || IR_Data[6] || IR_Data[7]) {
//                     curve_mode = 1; 
//                     curve_hold_timer = 40; // 弯道死锁时间稍微加长到 40，保证彻底转过弯心
//                 } 
//                 else if (abs_error < 0.8f) {
//                     // 只有车身基本回正了（误差小于 0.8），才允许开始倒计时退出弯道
//                     if (curve_hold_timer > 0) {
//                         curve_hold_timer--;
//                     } else {
//                         curve_mode = 0;
//                     }
//                 }

//                 float target_speed = curve_mode ? CURVE_SPEED : STRAIGHT_SPEED;

//                 if (current_base_speed < target_speed) {
//                     current_base_speed += (speed_mode == 1) ? 0.08f : 0.04f;  
//                     if (current_base_speed > target_speed) current_base_speed = target_speed;
//                 } else if (current_base_speed > target_speed) {
//                     // 🚨 加强制动：SAFE 模式下由 0.08 提升至 0.15，防止在官方纸上冲过头
//                     current_base_speed -= (speed_mode == 1) ? 0.40f : 0.15f;  
//                     if (current_base_speed < target_speed) current_base_speed = target_speed;
//                 }

//                 float error_d = error - last_pos_error;
//                 last_pos_error = error;
                
//                 float speed_adjust = 0.0f;
//                 if (abs_error <= 0.6f) {
//                     speed_adjust = (error * (TRACK_KP * 0.4f)) + (error_d * (TRACK_KD * 0.2f)); 
//                 } 
//                 else if (abs_error <= 1.5f) {
//                     speed_adjust = (error * (TRACK_KP * 0.7f)) + (error_d * (TRACK_KD * 0.5f));
//                 }
//                 else {
//                     speed_adjust = (error * TRACK_KP) + (error_d * TRACK_KD);
//                 }
                
//                 float max_adjust = 0.0f;
//                 if (curve_mode) {
//                     // 🚨 提升 SAFE 弯道扭矩：官方纸滑，把 SAFE 最大差速从 2.8 提到 3.4
//                     max_adjust = (speed_mode == 1) ? 4.5f : 3.4f; 
//                 } else {
//                     max_adjust = 2.5f; 
//                 }
                
//                 if(speed_adjust > max_adjust) speed_adjust = max_adjust;
//                 if(speed_adjust < -max_adjust) speed_adjust = -max_adjust;

//                 pid_left.target  = current_base_speed + speed_adjust;
//                 pid_right.target = current_base_speed - speed_adjust;
//             }
//         } 
//         else {
//             car_running = 0;
//             current_base_speed = 0.0f;
//             pid_left.target = 0;
//             pid_right.target = 0;
//             Motor_SetSpeed(0, 0); 
//         }

//         delay_cycles(160000); 
//     }
// }

// void delay_seconds(uint32_t s) {
//     for(uint32_t i = 0; i < s; i++) delay_cycles(32000000); 
// }

// // ========================================================
// // 🌟 底层执行：定时器 PID 中断
// // ========================================================
// void TIMER_PID_INST_IRQHandler(void)
// {
//     switch (DL_TimerG_getPendingInterrupt(TIMER_PID_INST)) {
//         case DL_TIMER_IIDX_ZERO:
//         {   
//             float raw_speed_L = (float)left_pulse_count;
//             float raw_speed_R = (float)right_pulse_count;
//             left_pulse_count = 0;
//             right_pulse_count = 0;

//             static uint16_t startup_cnt = 0;
//             static float filtered_speed_L = 0;
//             static float filtered_speed_R = 0;

//             if (car_running == 0) {
//                 Motor_SetSpeed(0, 0);
//                 pid_left.integral = 0;
//                 pid_right.integral = 0;
//                 startup_cnt = 0; 
//                 filtered_speed_L = 0;
//                 filtered_speed_R = 0;
//             } else {
//                 if(startup_cnt < 100) startup_cnt++;

//                 filtered_speed_L = 0.3f * raw_speed_L + 0.7f * filtered_speed_L;
//                 filtered_speed_R = 0.3f * raw_speed_R + 0.7f * filtered_speed_R;

//                 float pid_adj_L = PID_Calc(&pid_left, filtered_speed_L);
//                 float pid_adj_R = PID_Calc(&pid_right, filtered_speed_R);

//                // ========================================================
//                 // 🚨 终极底层引擎：双向静摩擦力绝对补偿
//                 // ========================================================
//                 float base_pwm_L = 0;
//                 float base_pwm_R = 0;

//                 // --- 左轮逻辑 ---
//                 if (pid_left.target > 0.05f) {
//                     // 正转补偿
//                     base_pwm_L = 24.0f + pid_left.target * 2.0f; 
//                 } else if (pid_left.target < -0.05f) {
//                     // 🚨 关键修复：反转补偿！负号方向也必须补足 -24.0 才能打破静摩擦！
//                     base_pwm_L = -24.0f + pid_left.target * 2.0f; 
//                 }

//                 // --- 右轮逻辑 ---
//                 if (pid_right.target > 0.05f) {
//                     // 正转补偿
//                     base_pwm_R = 48.0f + pid_right.target * 2.0f; 
//                 } else if (pid_right.target < -0.05f) {
//                     // 🚨 关键修复：反转补偿！负号方向补足 -48.0！
//                     base_pwm_R = -48.0f + pid_right.target * 2.0f; 
//                 }
                
//                 float pwm_out_L = base_pwm_L + pid_adj_L;
//                 float pwm_out_R = base_pwm_R + pid_adj_R;

//                 // ========================================================
//                 // 🛡️ 全向电机死区保护（防止在极低速时电机啸叫不转）
//                 // ========================================================
//                 if(pwm_out_L > 0 && pwm_out_L < 8.0f) pwm_out_L = 8.0f;
//                 if(pwm_out_L < 0 && pwm_out_L > -8.0f) pwm_out_L = -8.0f;
                
//                 if(pwm_out_R > 0 && pwm_out_R < 8.0f) pwm_out_R = 8.0f;
//                 if(pwm_out_R < 0 && pwm_out_R > -8.0f) pwm_out_R = -8.0f;

//                 Motor_SetSpeed((int8_t)pwm_out_L, (int8_t)pwm_out_R);
//             }
//             break;
//         }
//         default: break;
//     }
// }