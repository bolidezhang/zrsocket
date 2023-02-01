#include "biz_stage_handler.h"
#include "test_seda.h"

int BizStageHandler::handle_open()
{
    zrsocket::SedaStageThreadLRUSlotInfo slot[2];
    slot[0].capacity    = 10;
    slot[0].interval_ms = 100;
    slot[1].capacity    = 10;
    slot[1].interval_ms = 1000;
    stage_thread_->enable_lru_timers(slot, 2);
    counter_timer_ = stage_thread_->set_lru_timer(0, 0);
    //counter_timer_s_ = stage_thread_->set_lru_timer(1, 0);
    return 0;
}

int BizStageHandler::handle_event(const zrsocket::SedaEvent *event)
{
    TestApp &app = TestApp::instance();

    switch (event->type_) {
        case zrsocket::SedaEventTypeId::TIMER_EXPIRE:
            {
                zrsocket::SedaTimerExpireEvent *timer_event = (zrsocket::SedaTimerExpireEvent *)event;

                if (timer_event->slot == 0) {
                    //auto current_timestamp = zrsocket::OSApi::timestamp();
                    //printf("BizStageHandler::handle_event:TIMER_EXPIRE timestamp:%lld\n", current_timestamp);
                    if (app.push_end_.load(std::memory_order_relaxed)) {
                        zrsocket::uint_t handle_num = app.handle_num_.load(std::memory_order_relaxed);
                        if (handle_num >= app.push_num_.load(std::memory_order_relaxed)) {
                            app.push_end_.store(false, std::memory_order_relaxed);
                            app.test_counter_.update_end_counter();
                            //stage_thread_->cancel_lru_timer(1, counter_timer_s_);
                            printf("BizStageHandler::handle_event:TIMER_EXPIRE(slot:0) total_num_times:%ld spend_time:%lld us\n", handle_num, app.test_counter_.diff()/1000LL);
                            return 0;
                        }
                    }
                    counter_timer_ = stage_thread_->set_lru_timer(0, 0);
                }
                else {
                    zrsocket::uint_t handle_num = app.handle_num_.load(std::memory_order_relaxed);
                    printf("BizStageHandler::handle_event:TIMER_EXPIRE(slot:1) local_handle_num:%ld total_num_times:%ld timestamp:%lld\n", 
                        handle_num_, handle_num, zrsocket::SystemClockCounter::now());
                    counter_timer_s_ = stage_thread_->set_lru_timer(1, 0);
                }

            }
            break;
        case TestEventType::EVENT_TEST8:
            {
                //const Test8SedaEvent *test8_event = static_cast<const Test8SedaEvent *>(event);
                ++handle_num_;
                app.handle_num_.fetch_add(1, std::memory_order_relaxed);
                if (app.push_end_.load(std::memory_order_relaxed)) {
                    zrsocket::uint_t handle_num = app.handle_num_.load(std::memory_order_relaxed);
                    if (handle_num >= app.push_num_.load(std::memory_order_relaxed)) {
                        app.push_end_.store(false);
                        app.test_counter_.update_end_counter();
                        printf("BizStageHandler::handle_event:EVENT_TEST8 total_num_times:%ld spend_time:%lld us\n", handle_num, app.test_counter_.diff()/1000LL);

                        stage_thread_->cancel_lru_timer(0, counter_timer_);
                        stage_thread_->cancel_lru_timer(1, counter_timer_s_);
                    }
                }
            }
            break;
        case TestEventType::EVENT_TEST16:
            {
                //const Test16SedaEvent * test16_event = static_cast<const Test16SedaEvent *>(event);
                app.handle_num_.fetch_add(1, std::memory_order_relaxed);
                if (app.push_end_.load(std::memory_order_relaxed)) {
                    zrsocket::uint_t handle_num = app.handle_num_.load(std::memory_order_relaxed);
                    if (handle_num >= app.push_num_.load(std::memory_order_relaxed)) {
                        app.push_end_.store(false, std::memory_order_relaxed);
                        app.test_counter_.update_end_counter();
                        printf("BizStageHandler::handle_event EVENT_TEST16 total_num_times:%ld spend_time:%lld us\n", handle_num, app.test_counter_.diff()/1000LL);

                        stage_thread_->cancel_lru_timer(0, counter_timer_);
                        stage_thread_->cancel_lru_timer(1, counter_timer_s_);
                    }
                }
            }
            break;
        default:
            {
                thread_local int default_num = 0;
                ++default_num;
                printf("default_num:%ld, event->type:%d event->len:%d\n", default_num, event->type_, event->len_);
            }
            break;
    }

    return 0;
}
