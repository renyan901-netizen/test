#include "ctrl.h"
#include "ti_msp_dl_config.h"
#include "pid.h"
#include "track.h"
#include "motor.h"
#include "gimbal.h"
#include "k230.h"
#include "key.h" // 确保引入 KEY 定义

// ==========================================
// 🚗 底盘私有参数与变量 (原版稳定参数)
// ==========================================
#define SPEED_KP 1.0f  
#define SPEED_KI 0.2f  
#define SPEED_KD 0.0f  

#define TRACK_KP 2.8f  
#define TRACK_KD 20.0f 

#define CORNERS_PER_LAP 4  
#define WHITE_TIMEOUT 60 

PID_TypeDef pid_left;
PID_TypeDef pid_right;

extern volatile int32_t left_pulse_count;
extern volatile int32_t right_pulse_count;

volatile uint8_t car_running = 0;  
float STRAIGHT_SPEED = 3.0f;   
float CURVE_SPEED    = 1.5f;   

// 暴露给全局的 UI 变量
uint8_t  target_laps = 1;      
uint8_t  speed_mode  = 0;      
uint8_t  start_flag  = 0;      
uint16_t corner_count   = 0;   

// 🚨 综合模式变量
uint8_t  sys_mode     = 0;  // 0=常规, 1=综合
uint8_t  track_enable = 1;  // 1=循迹开, 0=循迹关(仅打靶)

// 底盘状态变量
static uint16_t debounce_timer = 0;   
static uint32_t t_first = 0;      
static uint32_t t_full = 0;       
static uint32_t homing_ticks = 0; 
static uint8_t  track_state = 0;  
static float current_base_speed = 0.0f; 
static float last_valid_pos = 4.5f; 
static float last_pos_error = 0.0f; 
static uint32_t white_area_cnt = 0;
static uint8_t curve_mode = 0; 

// ==========================================
// 🎯 云台私有参数与变量
// ==========================================
PID_TypeDef pid_test_x;
PID_TypeDef pid_test_y;
static uint16_t target_lost_timeout = 0;

// ==========================================
// 🛠️ 内部私有函数
// ==========================================
static float Get_Track_Position(void) {
    float sum = 0;
    int active_cnt = 0;
    for(int i = 0; i < 8; i++) {
        if(IR_Data[i]) { 
            sum += (i + 1.0f);
            active_cnt++;
        }
    }
    if(active_cnt == 0) return 0.0f; 
    return sum / active_cnt;
}

// ==========================================
// 🚀 核心控制接口实现
// ==========================================

void Ctrl_Init(void) {
    PID_Init(&pid_left, SPEED_KP, SPEED_KI, SPEED_KD);
    PID_Init(&pid_right, SPEED_KP, SPEED_KI, SPEED_KD);
    
    PID_Init(&pid_test_x, 9.0f, 0.0f, 1.5f);
    PID_Init(&pid_test_y, 9.0f, 0.0f, 1.5f);
}

void Ctrl_Key_Process(uint8_t key_val) {
    if(key_val == 0) return;

//    // 新增 K4：综合模式切换
//     if(key_val == KEY4_PRES) {
//         sys_mode = !sys_mode;
//         if(sys_mode == 1) {
//             speed_mode = 0; 
//             STRAIGHT_SPEED = 3.0f;
//             // 🚨 终极降速补丁：把 1.5f 降到 1.0f 甚至 0.8f！
//             // 哪怕它像蜗牛一样转弯，只要靶子在视野里，它就赢了！
//             CURVE_SPEED    = 1.0f; 
//             track_enable   = 1; 
//         } else {
//             track_enable = 1;
//         }
//     }

// 新增 K4：综合模式切换
    if(key_val == KEY4_PRES) {
        sys_mode = !sys_mode;
        if(sys_mode == 1) {
            speed_mode = 0; 
            STRAIGHT_SPEED = 4.0f; // 直道再快一点
            // 🚨 物理定律不可违背：0.8 依然会卡死！必须锁死 1.0！
            CURVE_SPEED    = 1.0f; 
            track_enable   = 1; 
        } else {
            track_enable = 1;
        }
    }
    // K1：切圈数
    else if(key_val == KEY1_PRES) {
        target_laps++;
        if(target_laps > 5) target_laps = 0; 
    }
    // K2：模式依赖按键
    else if(key_val == KEY2_PRES) {
        if (sys_mode == 0) {
            speed_mode = !speed_mode;
            if(speed_mode == 1) {
                STRAIGHT_SPEED = 5.0f;  
                CURVE_SPEED    = 2.5f;
            } else {
                STRAIGHT_SPEED = 3.0f;  
                CURVE_SPEED    = 1.5f;
            }
        } else {
            track_enable = !track_enable;
        }
    }
    // K3：发车/急停
    else if(key_val == KEY3_PRES) {
        if(start_flag == 0) {
            start_flag = 1;      
            car_running = 1;
            corner_count = 0;    
            debounce_timer = 0;  
            track_state = 1; 
            t_first = 0;
            t_full = 0;
            homing_ticks = 0;
            current_base_speed = 0.5f; 
        } else {
            start_flag = 0;      
            car_running = 0;
            track_state = 0;
        }
    }
}

void Ctrl_Chassis_Process(void) {
    if (start_flag == 1) {
        if (track_enable == 0) {
            car_running = 0;
            current_base_speed = 0.0f;
            pid_left.target = 0;
            pid_right.target = 0;
            return; 
        }

        if (track_state == 1) t_first++;
        else if (track_state == 2) t_full++;
        else if (track_state == 4) homing_ticks++;

        ReadEightIR();
        
        // ========================================================
        // 🚨 唯一增加的补丁：强行物理容错，屏蔽幽灵探头！
        // ========================================================
        // 左转时（左边压线），强行无视最右侧 7 号探头的任何信号
        if (IR_Data[0] || IR_Data[1] || IR_Data[2]) {
            IR_Data[7] = 0; 
        }
        // 右转时（右边压线），强行无视最左侧 0 号探头的任何信号
        if (IR_Data[5] || IR_Data[6] || IR_Data[7]) {
            IR_Data[0] = 0;
        }
        // ========================================================

        int black_cnt = 0;
        for(int i=0; i<8; i++) if(IR_Data[i]) black_cnt++;
        
        float current_pos = Get_Track_Position();

        if (debounce_timer > 0) debounce_timer--;

        if (black_cnt >= 4 || (IR_Data[0] == 1 && IR_Data[1] == 1 && IR_Data[2] == 1)) { 
            if (debounce_timer == 0) {
                corner_count++;
                debounce_timer = 200; // 原版 100 防抖
                
                if (track_state == 1) track_state = 2;       
                else if (track_state == 2) track_state = 3;  
                
                uint16_t target_corners = (target_laps == 0) ? 2 : (target_laps * CORNERS_PER_LAP);

                if (corner_count >= target_corners) {
                    track_state = 4; 
                }
            }
        }

        int32_t target_ticks = (int32_t)t_full - (int32_t)t_first +25;
        if (target_ticks < 0) target_ticks = 0; 
        
        if (track_state == 4 && homing_ticks >= target_ticks) {
            start_flag = 0;            
            car_running = 0;           
            track_state = 0;
            current_base_speed = 0.0f; 
            pid_left.target  = 0;
            pid_right.target = 0;
        }
        else {
            if(car_running == 0) {
                car_running = 1;
                current_base_speed = 0.5f; 
            }
            
            if(current_pos == 0.0f) {
                white_area_cnt++;
                if(white_area_cnt > WHITE_TIMEOUT) {
                    car_running = 0; start_flag = 0; track_state = 0;
                    current_base_speed = 0.0f; 
                }
                current_pos = last_valid_pos; 
            } else {
                white_area_cnt = 0;
                last_valid_pos = current_pos; 
            }

            static float smoothed_error = 0.0f;
            float raw_error = current_pos - 4.5f; 
            smoothed_error = 0.6f * raw_error + 0.4f * smoothed_error; 
            float error = smoothed_error; 
            float abs_error = (error > 0) ? error : -error;

            if (IR_Data[0] || IR_Data[1] || IR_Data[6] || IR_Data[7] || abs_error > 1.2f) {
                curve_mode = 1; 
            } else if (!IR_Data[0] && !IR_Data[1] && !IR_Data[6] && !IR_Data[7] && abs_error < 0.5f) {
                curve_mode = 0;
            }

            float target_speed = curve_mode ? CURVE_SPEED : STRAIGHT_SPEED;

            if (current_base_speed < target_speed) {
                if (current_base_speed < 1.2f) current_base_speed = 1.2f; 
                current_base_speed += (speed_mode == 1) ? 0.08f : 0.04f;  
                if (current_base_speed > target_speed) current_base_speed = target_speed;
            } else if (current_base_speed > target_speed) {
                // 提高刹车力度以适应更高的直道速度，确保能及时减速到 0.6
                current_base_speed -= (speed_mode == 1) ? 0.60f : 0.40f;  
                if (current_base_speed < target_speed) current_base_speed = target_speed;
            }

            float error_d = error - last_pos_error;
            last_pos_error = error;
            
            float speed_adjust = 0.0f;
            if (abs_error <= 0.6f) {
                speed_adjust = (error * (TRACK_KP * 0.4f)) + (error_d * (TRACK_KD * 0.2f)); 
            } else if (abs_error <= 1.5f) {
                speed_adjust = (error * (TRACK_KP * 0.7f)) + (error_d * (TRACK_KD * 0.5f));
            } else {
                speed_adjust = (error * TRACK_KP) + (error_d * TRACK_KD);
            }
            
           float max_adjust = 0.0f;
            if (curve_mode) {
                if (sys_mode == 1) {
                    // 线性转弯，不要急速转弯：恢复到 1.4，1.2 差速太小会导致外侧轮推不动内侧轮死区
                    max_adjust = 1.4f; 
                } else {
                    max_adjust = (speed_mode == 1) ? 4.5f : 2.8f; 
                }
            } else {
                max_adjust = 2.5f; 
            }
            
            if(speed_adjust > max_adjust) speed_adjust = max_adjust;
            if(speed_adjust < -max_adjust) speed_adjust = -max_adjust;

            pid_left.target  = current_base_speed + speed_adjust;
            pid_right.target = current_base_speed - speed_adjust;
        }
    } else {
        car_running = 0;
        current_base_speed = 0.0f;
        pid_left.target = 0;
        pid_right.target = 0;
        Motor_SetSpeed(0, 0); 
    }
}

void Ctrl_Gimbal_Process(void) {
    if (sys_mode == 0) {
        Gimbal_SetSpeed_X(0);
        Gimbal_SetSpeed_Y(0);
        pid_test_x.integral = 0; 
        pid_test_y.integral = 0;
        return; 
    }

    if (k230_update_flag == 1) {
        k230_update_flag = 0;
        if (k230_target_x < 5 && k230_target_y < 5) {
            if (target_lost_timeout < 100) target_lost_timeout++; 
        } 
        else {
            target_lost_timeout = 0; 
            static float smooth_x = 320.0f;
            static float smooth_y = 240.0f;
            
            float alpha = 0.85f; 
            smooth_x = (alpha * k230_target_x) + ((1.0f - alpha) * smooth_x);
            smooth_y = (alpha * k230_target_y) + ((1.0f - alpha) * smooth_y);

            float LASER_OFFSET_X = 30.0f; 
            float LASER_OFFSET_Y = -15.0f;   
            
            // 🚨 弯道动态预判机制：告别死板的 80px！
            // 使用底盘的真实差速来计算预偏量。
            if (curve_mode == 1) {
                float chassis_diff = pid_left.target - pid_right.target;
                // 🚨 最后的微调！
                // 14 能完美解决 3 个弯道，剩下 1 个最极端的弯还有一丝向左（提前量差一点点）
                // 提升到 15！榨干这最后的误差！
                LASER_OFFSET_X += (chassis_diff * 15.0f);
            }

            float offset_x = (320.0f - smooth_x) + LASER_OFFSET_X; 
            float offset_y = (240.0f - smooth_y) + LASER_OFFSET_Y; 

            if (offset_x > -8.0f && offset_x < 8.0f) offset_x = 0;
            if (offset_y > -8.0f && offset_y < 8.0f) offset_y = 0;

            float abs_offset_x = (offset_x > 0) ? offset_x : -offset_x;
            
            // 线性转弯：废除 40px 断崖，改为绝对平滑的线性 Kp
            // 进一步提升极限爆发力！即使在背对靶子的极限弯道，只要快脱靶，直接爆发出 22.0 的死锁力拽回来！
            float dynamic_kp_x = 3.5f + 18.5f * (abs_offset_x / 80.0f);
            if (dynamic_kp_x > 22.0f) dynamic_kp_x = 22.0f;
            pid_test_x.Kp = dynamic_kp_x;
            pid_test_x.Kd = 4.5f;

            pid_test_y.Kp = 4.0f;
            pid_test_y.Kd = 2.5f;

            float speed_x = PID_Calc(&pid_test_x, offset_x);
            float speed_y = PID_Calc(&pid_test_y, offset_y);

            float max_gimbal_speed = 850.0f; 
            if(speed_x > max_gimbal_speed)  speed_x = max_gimbal_speed;
            if(speed_x < -max_gimbal_speed) speed_x = -max_gimbal_speed;
            if(speed_y > max_gimbal_speed)  speed_y = max_gimbal_speed;
            if(speed_y < -max_gimbal_speed) speed_y = -max_gimbal_speed;

            Gimbal_SetSpeed_X(speed_x);
            Gimbal_SetSpeed_Y(speed_y);
        }
    } else {
        if (target_lost_timeout < 100) target_lost_timeout++;
    }
    
    if (target_lost_timeout >= 100) {
        Gimbal_SetSpeed_X(0);
        Gimbal_SetSpeed_Y(0);
        pid_test_x.integral = 0; 
        pid_test_y.integral = 0;
    }
}

void Ctrl_Motor_IRQ_Process(void) {
    float raw_speed_L = (float)left_pulse_count;
    float raw_speed_R = (float)right_pulse_count;
    left_pulse_count = 0;
    right_pulse_count = 0;

    static float filtered_speed_L = 0;
    static float filtered_speed_R = 0;

    if (car_running == 0) {
        Motor_SetSpeed(0, 0);
        pid_left.integral = 0;
        pid_right.integral = 0;
        filtered_speed_L = 0;
        filtered_speed_R = 0;
    } else {
        filtered_speed_L = 0.3f * raw_speed_L + 0.7f * filtered_speed_L;
        filtered_speed_R = 0.3f * raw_speed_R + 0.7f * filtered_speed_R;

        float pid_adj_L = PID_Calc(&pid_left, filtered_speed_L);
        float pid_adj_R = PID_Calc(&pid_right, filtered_speed_R);

        float base_pwm_L = 0;
        float base_pwm_R = 0;

        // 原版基础推力和 6.0 倒转拉力
        if (pid_left.target > 0) {
            base_pwm_L = 24.0f + pid_left.target * 2.0f; 
        } else {
            base_pwm_L = pid_left.target * 6.0f; 
        }

        if (pid_right.target > 0) {
            base_pwm_R = 48.0f + pid_right.target * 2.0f; 
        } else {
            base_pwm_R = pid_right.target * 6.0f; 
        }

        float pwm_out_L = base_pwm_L + pid_adj_L;
        float pwm_out_R = base_pwm_R + pid_adj_R;

        if(pwm_out_L < 8.0f && pid_left.target > 0) pwm_out_L = 8.0f;
        if(pwm_out_R < 8.0f && pid_right.target > 0) pwm_out_R = 8.0f;

        Motor_SetSpeed((int8_t)pwm_out_L, (int8_t)pwm_out_R);
    }
}