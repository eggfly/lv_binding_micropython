/**
 * @file hal.cpp
 * @author Forairaaaaa
 * @brief 
 * @version 0.1
 * @date 2023-05-20
 * 
 * @copyright Copyright (c) 2023
 * 
 */
#include "hal.h"
#include "hal_config.h"



void HAL::init()
{
    /* Display */
    disp.init();
    disp.setColorDepth(16);
    disp.setBrightness(BRIGHTNESS);


    /* Touch pad and I2C port 0 (default) */
    // auto cfg = tp.config();
    // cfg.pull_up_en = false;
    // tp.config(cfg);
    // tp.init(HAL_PIN_I2C_SDA, HAL_PIN_I2C_SCL, HAL_PIN_TP_RST, HAL_PIN_TP_INTR, true, 400000);


    /* PMU AXP2101 */
    // pmu.init(HAL_PIN_I2C_SDA, HAL_PIN_I2C_SCL, HAL_PIN_AXP_INTR);


    /* Buttons */
    // btnA.begin();
    // btnB.begin();


    /* Once button and power setup, check boot mode */
    checkBootMode();


    /* RTC PCF8563 */
    // rtc.init(HAL_PIN_I2C_SDA, HAL_PIN_I2C_SCL, HAL_PIN_RTC_INTR);


    /* Buzzer */
    // buzz.init(HAL_PIN_BUZZER);

    /* SD card */
    // sd.init();


    /* Lvgl */
    lvgl.init(&disp, &tp);


    // /* IMU BMI270 + BMM150 */
    // imu.init();
    // /* Setup wrist wear wakeup interrupt */
    // imu.setWristWearWakeup();
    // /* Enable step counter */
    // imu.enableStepCounter();

    // jingle_bells();

}


void HAL::update()
{
    lvgl.update();
}

#define delay(ms) vTaskDelay(pdMS_TO_TICKS(ms))



const std::string disk_ascii = R"(
   ****     ### *
 ******     ### ****
 ******         *****
 ********************
 ****/,,,,,,,,,,,/***
 ****             ***
 ****             ***
 ****             ***
)";

const std::string apple_ascii = R"(
                        @@@@                 
                     @@@@@@@                 
                    @@@@@@%                  
                   ,@@@@                     
        .@@@@@@@@@    @@@@@@@@@@&            
      @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@         
    @@@@@@@@@@@@@@@@@@@@@@@@@@@@@%           
   @@@@@@@@@@@@@@@@@@@@@@@@@@@@@             
   @@@@@@@@@@@@@@@@@@@@@@@@@@@@%             
   @@@@@@@@@@@@@@@@@@@@@@@@@@@@&             
   @@@@@@@@@@@@@@@@@@@@@@@@@@@@@,            
    @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@           
    &@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@        
      @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@         
       @@@@@@@@@@@@@@@@@@@@@@@@@@@           
         @@@@@@@@@@@@@@@@@@@@@@@             
            @@@@         @@@@                
)";

const std::string apple_ascii_2 = R"(        
             @@         
           +@@@         
           @@@          
          @@@@          
          @@+           
     @@@+   *@@@-       
   @@@@@@@@@@@@@@@      
  @@@@@@@@@@@@@@@@@     
  @@@@@@@@@@@@@@@@      
 .@@@@@@@@@@@@@@@       
 @@@@@@@@@@@@@@@@       
 @@@@@@@@@@@@@@@@       
 @@@@@@@@@@@@@@@@       
 @@@@@@@@@@@@@@@@       
  @@@@@@@@@@@@@@@@      
  @@@@@@@@@@@@@@@@@     
  @@@@@@@@@@@@@@@@@     
   @@@@@@@@@@@@@@@@     
   @@@@@@@@@@@@@@@      
    @@@@@@@@@@@@@-      
     @@@@@@@@@@@@       
      @@     @@                          
)";

const std::string apple_ascii_3 = R"(    
          -#@+        
         =@@+         
   .=**+-=++**+-      
  *@@@@@@@@@@@@@#     
 *@@@@@@@@@@@@@+      
 %@@@@@@@@@@@@@:      
 +@@@@@@@@@@@@@#.     
 .@@@@@@@@@@@@@@@=    
  .%@@@@@@@@@@@@#     
    +@@@#**%@@%=      
)";

const std::string apple_ascii_4 = R"(
              .MWz          
            .B$$$           
           :W$$q'           
      CmmL..+<?mmmm,        
   X$$$$$$$$$$$$$$$$$p      
  p$$$$$$$$$$$$$$$$$t       
 r$$$$$$$$$$$$$$$$$z        
 k$$$$$$$$$$$$$$$$$c        
 ($$$$$$$$$$$$$$$$$w.       
  @$$$$$$$$$$$$$$$$$@}      
  [@$$$$$$$$$$$$$$$$$$;     
   .$$$$$$$$$$$$$$$$$(      
     b$$$$$$$$$$$$$&.       
       MM1.    ?MX.         
)";

const std::string i_love_dos = R"(
              ,----------------,              ,---------,

         ,-----------------------,          ,"        ,"|

       ,"                      ,"|        ,"        ,"  |

      +-----------------------+  |      ,"        ,"    |

      |  .-----------------.  |  |     +---------+      |

      |  |                 |  |  |     | -==----'|      |

      |  |  I LOVE DOS!    |  |  |     |         |      |

      |  |  Bad command or |  |  |/----|`---=    |      |

      |  |  C:\>_          |  |  |   ,/|==== ooo |      ;

      |  |                 |  |  |  // |(((( [33]|    ,"

      |  `-----------------'  |," .;'| |((((     |  ,"

      +-----------------------+  ;;  | |         |,"

         /_)______________(_/  //'   | +---------+

    ___________________________/___  `,

   /  oooooooooooooooo  .o.  oooo /,   \,"-----------

  / ==ooooooooooooooo==.o.  ooo= //   ,`\--{)B     ,"

 /_==__==========__==_ooo__ooo=_/'   /___________,"
)";

const std::string i_love_dos2 = R"(
         ,----------------,
    ,-----------------------,
  ,"                      ,"|
 +-----------------------+  |
 |  .-----------------.  |  |
 |  |                 |  |  |
 |  |  I LOVE DOS!    |  |  |
 |  |  Bad command or |  |  |
 |  |  C:\>_          |  |  |
 |  |                 |  |  |
 |  `-----------------'  |,"
 +-----------------------+
    /_)______________(_/  //'
   _________________________/
  /  oooooooooooo  .o.  oo /,
 / ==ooooooooooo==.o.  o= //
/_==__======__==_ooo__oo_/'
)";


void HAL::checkBootMode()
{
    // /* Press button B while power on to enter USB MSC mode */
    // if (!btnB.read()) {
    //     vTaskDelay(pdMS_TO_TICKS(20));
    //     if (!btnB.read()) {

    //         disp.fillScreen(TFT_BLACK);
    //         disp.setTextSize(3);
    //         disp.setCursor(0, 30);
    //         disp.printf(" :)\n Release Key\n To Enter\n USB MSC Mode\n");

    //         /* Wait release */
    //         while (!btnB.read()) {
    //             vTaskDelay(pdMS_TO_TICKS(10));
    //         }

    //         disp.fillScreen(TFT_BLACK);
    //         disp.setTextSize(3);
    //         disp.setCursor(0, 40);
    //         // disp.printf(" USB MSC Mode\n\n\n\n\n\n\n\n\n\n Reboot     ->");
    //         disp.printf(" [ USB MSC Mode ]");

    //         disp.setCursor(0, 50);
    //         disp.setTextSize(2);
    //         disp.printf("\n%s", i_love_dos2.c_str());

    //         disp.setTextSize(2);
    //         disp.printf("\n Press to quit            ->");

    //         /* Enable usb msc */
    //         hal_enter_usb_msc_mode();


    //         /* Simply restart make usb not vailable, dont know why */
    //         pmu.powerOff();
    //         while (1) {
    //             vTaskDelay(1000);
    //         }

    //     }
    // }

}