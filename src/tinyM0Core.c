#include "tinyM0Core.h"
#include "tinyM0Config.h"

tinyThread_t tinyThread[sizeof(tinyThreadConfig) / sizeof(tinyThreadConfig_t)];

void osStart(void){
    static uint8_t currentThread;
    while(1){
        switch(tinyThread[currentThread].state){
            case NEW:
                tinyThread[currentThread].state = RUN;
                tinyThreadConfig[currentThread].proc();
                break;
            case RUN:
                break;
        }
    }
}

void yield(void){

}
