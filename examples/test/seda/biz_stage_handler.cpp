#include "biz_stage_handler.h"
#include "test_seda.h"

int BizStageHandler::handle_open()
{
    zrsocket::SedaStageThreadLRUSlotInfo slot[1];
    slot[0].capacity    = 10;
    slot[0].interval_ms = 100;
    stage_thread_->enable_lru_timers(slot, 1);
    timer_ = stage_thread_->set_lru_timer(0, 0);
    return 0;
}

int BizStageHandler::handle_event(const zrsocket::SedaEvent *event)
{
    TestApp &app = TestApp::instance();

    switch (event->type()) {
        case zrsocket::SedaEventTypeId::TIMER_EXPIRE:
            {
                //auto current_timestamp = zrsocket::OSApi::timestamp();
                //printf("BizStageHandler::handle_event:TIMER_EXPIRE timestamp:%lld\n", current_timestamp);
                if (app.push_end_.load(std::memory_order_relaxed)) {
                    int handle_num = app.handle_num_.load(std::memory_order_relaxed);
                    if (handle_num >= app.push_num_.load(std::memory_order_relaxed)) {
                        app.push_end_.store(false, std::memory_order_relaxed);
                        auto current_timestamp = zrsocket::OSApi::timestamp();
                        printf("BizStageHandler::handle_event:TIMER_EXPIRE total_num_times:%ld spend_time:%lld us\n", handle_num, current_timestamp - app.startup_test_timestamp_);
                        return 0;
                    }
                }
                timer_ = stage_thread_->set_lru_timer(0, 0);
            }
            break;
        case TestEventType::EVENT_TEST8:
            {
                //const Test8SedaEvent *test8_event = static_cast<const Test8SedaEvent *>(event);
                app.handle_num_.fetch_add(1, std::memory_order_relaxed);
                if (app.push_end_.load(std::memory_order_relaxed)) {
                    int handle_num = app.handle_num_.load(std::memory_order_relaxed);
                    if (handle_num >= app.push_num_.load(std::memory_order_relaxed)) {
                        app.push_end_.store(false, std::memory_order_relaxed);
                        auto current_timestamp = zrsocket::OSApi::timestamp();
                        printf("BizStageHandler::handle_event:EVENT_TEST8 total_num_times:%ld spend_time:%lld us\n", handle_num, current_timestamp - app.startup_test_timestamp_);

                        stage_thread_->cancel_lru_timer(0, timer_);
                    }
                }
            }
            break;
        case TestEventType::EVENT_TEST16:
            {
                //const Test16SedaEvent * test16_event = static_cast<const Test16SedaEvent *>(event);
                app.handle_num_.fetch_add(1, std::memory_order_relaxed);
                if (app.push_end_.load(std::memory_order_relaxed)) {
                    int handle_num = app.handle_num_.load(std::memory_order_relaxed);
                    if (handle_num >= app.push_num_.load(std::memory_order_relaxed)) {
                        app.push_end_.store(false, std::memory_order_relaxed);
                        auto current_timestamp = zrsocket::OSApi::timestamp();
                        printf("BizStageHandler::handle_event EVENT_TEST16 total_num_times:%ld spend_time:%lld us\n", handle_num, current_timestamp - app.startup_test_timestamp_);

                        stage_thread_->cancel_lru_timer(0, timer_);
                    }
                }
            }
            break;
        default:
            break;
    }

    return 0;
}
