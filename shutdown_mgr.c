
#include <stdint.h>
#include "mcc_generated_files/system/system.h"
#include "tasks.h"
#include "power_mgr.h"


extern uint64_t GetTimeMs();
extern uint8_t CLIENT_DATA[];
extern uint64_t pi_run_last_falling_time_ms;
extern volatile uint64_t timer_blink_period ;



//0x55 means shutdown has been requested from PIC to PI
#define REG_SHUTDOWN_REQ_SET() do { CLIENT_DATA[4]=0x55; } while(0);
#define REG_SHUTDOWN_REQ_RESET() do { CLIENT_DATA[4]=0x0; } while(0);
#define REG_SHUTDOWN_ACKED_RESET() do { CLIENT_DATA[5]=0x0; } while(0);
//PI will write 0xAA if shutdown req processed
#define IS_REG_SHUTDOWN_ACKED()  (CLIENT_DATA[5]==0xAA)

//
#define PI_RUN_TMOUT_MS 2000 
#define MCU_INT_PIN_SET_TIME_MS 200

static void PIShutdownMonitor(volatile struct TaskDescr* taskd);

typedef struct {
    uint64_t btn_pressed_time_ms; //when long QON button press happened
    bool mcu_int_pin_set; // if MCU INT pin has been SET to 1
    uint64_t mcu_int_pin_set_time_ms; //time when MCU INT pin will be SET to 1
    uint64_t shutdown_prcess_time_ms; //time when shutdown process started
    bool shutdown_in_progress; //shutdown procedure is in progress
}ShutdownData_t;
volatile ShutdownData_t sht_data;

void ShutdownMgrInit(void){
    sht_data.btn_pressed_time_ms = 0;
    sht_data.mcu_int_pin_set = false;
    sht_data.mcu_int_pin_set_time_ms = 0;
    sht_data.shutdown_in_progress = false;
}

void ShutdownButtonPressed(void){
    
    sht_data.shutdown_in_progress = true;
    sht_data.shutdown_prcess_time_ms = GetTimeMs();
    
    //sign shutdown request in REG
    REG_SHUTDOWN_REQ_SET();
    
    //PI will ack it via REG, so reset it before
    REG_SHUTDOWN_ACKED_RESET();
    
    //save current time
    sht_data.btn_pressed_time_ms = GetTimeMs();
    
    //SET MCU INT generate falling edge interrupt for PI
    MCU_INT_N_SetLow();
    //SET MCU INT pin after certain time, PIShutdownMonitor will handle
    sht_data.mcu_int_pin_set=false;
    sht_data.mcu_int_pin_set_time_ms=GetTimeMs() + MCU_INT_PIN_SET_TIME_MS;
    
    add_task(TASK_PI_SHUTDOWN_MONITOR, PIShutdownMonitor,(void*) &sht_data);
    

}

static void PIShutdownMonitor(volatile struct TaskDescr* taskd){  
    
    
    ShutdownData_t* sht_data_p  = taskd->task_state;
    
    uint64_t now = GetTimeMs();
    if(sht_data_p->shutdown_in_progress){
        if(now > (sht_data_p->shutdown_prcess_time_ms + 10000)){
            //too much time.. PI not answer...
            //what to do
//            timer_blink_period=5000;
        }
    }

             
    //check if time elapsed to SET MCU INT pin to 1
    if(!sht_data_p->mcu_int_pin_set){
         if(now > sht_data_p->mcu_int_pin_set_time_ms){
             MCU_INT_N_SetHigh();
             sht_data_p->mcu_int_pin_set=true;
         }
    }

    //check shutdown acked register
    if(IS_REG_SHUTDOWN_ACKED()){
        //check if falling edge happened too long ago
        if((pi_run_last_falling_time_ms + PI_RUN_TMOUT_MS) < now){
            //timeout, PI is not sending HEARTBEAT for a while
            //it has been shut down
            
            //clear REGS
            REG_SHUTDOWN_REQ_RESET();
            REG_SHUTDOWN_ACKED_RESET();
            
            //go to shutdown mode
            //todo blink just for debug
            timer_blink_period=3000;
            int ret = PowMgrGoToShipMode();
            if (ret != 0){
                timer_blink_period=100;
            }
            
        }
        
    }
}
