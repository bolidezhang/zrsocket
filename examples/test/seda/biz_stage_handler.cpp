#include "biz_stage_handler.h"
#include "test_seda.h"

int BizStageHandler::handle_open()
{
    zrsocket::SedaStageThreadLRUSlotInfo slot[2];
    slot[0].capacity    = 10;
    slot[0].interval_ms = 100;
    slot[1].capacity    = 10;
    slot[1].interval_ms = 3000;
    stage_thread_->enable_lru_timers(slot, 2);
    counter_timer_ = stage_thread_->set_lru_timer(0, 0);
    counter_timer_s_ = stage_thread_->set_lru_timer(1, 0);
    return 0;
}

int BizStageHandler::handle_event(const zrsocket::SedaEvent *event)
{
    TestApp &app = TestApp::instance();

    int type = event->type();
    //int len  = event->event_len();
    //if (!((type == TestEventType::EVENT_TEST8) || (type == zrsocket::SedaEventTypeId::TIMER_EXPIRE))) {
    //    int type1 = event->type();
    //    int len1  = event->event_len();
    //    printf("type:%d, len:%d, type1:%d len1:%d\n", type, len, type1, len1);
    //}

    switch (type) {
        case zrsocket::SedaEventTypeId::TIMER_EXPIRE:
            {
                zrsocket::SedaTimerExpireEvent *timer_event = (zrsocket::SedaTimerExpireEvent *)event;

                if (timer_event->slot == 0) {
                    //auto current_timestamp = zrsocket::OSApi::timestamp();
                    //printf("BizStageHandler::handle_event:TIMER_EXPIRE timestamp:%lld\n", current_timestamp);
                    //if (app.push_end_.load(std::memory_order_relaxed)) {
                        zrsocket::uint_t app_handle_num = app.handle_num_.load(std::memory_order_relaxed);
                        if (app_handle_num >= app.push_num_.load(std::memory_order_relaxed)) {
                            app.push_end_.store(false, std::memory_order_relaxed);
                            app.test_counter_.update_end_counter();
                            //stage_thread_->cancel_lru_timer(1, counter_timer_s_);
                            printf("BizStageHandler::handle_event:TIMER_EXPIRE(slot:0) total_num_times:%ld spend_time:%lld ns\n", app_handle_num, app.test_counter_.diff());
                            //ZRSOCKET_LOG_INFO("BizStageHandler::handle_event:TIMER_EXPIRE(slot:0) total_num_times:" << app_handle_num
                            //    << " spend_time:" << static_cast<uint64_t>(app.test_counter_.diff() / 1000LL) << " us");
                            return 0;
                        }
                    //}
                    counter_timer_ = stage_thread_->set_lru_timer(0, 0);
                }
                else {
                    zrsocket::uint_t app_handle_num = app.handle_num_.load(std::memory_order_relaxed);

                    printf("BizStageHandler::handle_event:TIMER_EXPIRE(slot:1) local_handle_num:%ld total_num_times:%ld timestamp:%lld\n", handle_num_, app_handle_num, 
                        zrsocket::SystemClockCounter::now());

                    //ZRSOCKET_LOG_INFO("BizStageHandler::handle_event:TIMER_EXPIRE(slot:1) local_handle_num:" << handle_num_ <<" total_num_times:"
                    //    << app_handle_num <<" timestamp:"<<zrsocket::SystemClockCounter::now());

                    counter_timer_s_ = stage_thread_->set_lru_timer(1, 0);
                }
            }
            break;
        case TestEventType::EVENT_TEST8:
            {
                //const Test8SedaEvent *test8_event = static_cast<const Test8SedaEvent *>(event);
                ++handle_num_;
                ZRSOCKET_LOG_TRACE("seda handle_num:" << handle_num_);
                app.handle_num_.fetch_add(1, std::memory_order_relaxed);
                if (app.push_end_.load(std::memory_order_relaxed)) {
                    zrsocket::uint_t app_handle_num = app.handle_num_.load(std::memory_order_relaxed);
                    if (app_handle_num >= app.push_num_.load(std::memory_order_relaxed)) {
                        app.push_end_.store(false);
                        app.test_counter_.update_end_counter();
                        printf("BizStageHandler::handle_event EVENT_TEST8 total_num_times:%ld spend_time:%lld ns\n", app_handle_num, app.test_counter_.diff());
                        //ZRSOCKET_LOG_INFO("BizStageHandler::handle_event:EVENT_TEST8 total_num_times:" << app_handle_num << " spend_time:" << app.test_counter_.diff() << " ns");

                        stage_thread_->cancel_lru_timer(0, counter_timer_);
                        //stage_thread_->cancel_lru_timer(1, counter_timer_s_);
                    }
                }
            }
            break;
        case TestEventType::EVENT_TEST16:
            {
                //const Test16SedaEvent * test16_event = static_cast<const Test16SedaEvent *>(event);
                app.handle_num_.fetch_add(1, std::memory_order_relaxed);
                if (app.push_end_.load(std::memory_order_relaxed)) {
                    zrsocket::uint_t app_handle_num = app.handle_num_.load(std::memory_order_relaxed);
                    if (app_handle_num >= app.push_num_.load(std::memory_order_relaxed)) {
                        app.push_end_.store(false, std::memory_order_relaxed);
                        app.test_counter_.update_end_counter();
                        ZRSOCKET_LOG_INFO("BizStageHandler::handle_event:EVENT_TEST16 total_num_times:" << app_handle_num << " spend_time:" << app.test_counter_.diff() << " ns");
                        //printf("BizStageHandler::handle_event EVENT_TEST16 total_num_times:%ld spend_time:%lld us\n", app_handle_num, app_handle_num, app.test_counter_.diff());

                        stage_thread_->cancel_lru_timer(0, counter_timer_);
                        //stage_thread_->cancel_lru_timer(1, counter_timer_s_);
                    }
                }
            }
            break;
        default:
            {
                thread_local int default_num = 0;
                ++default_num;
                int type1 = event->type();
                int len1  = event->event_len();
                printf("default_num:%ld, event->type:%d event->len:%d\n", default_num, type1, len1);
            }
            break;
    }

    return 0;
}
