#include "biz_stage_handler.h"
#include "test_seda.h"

int BizStageHandler::handle_event(const zrsocket::SedaEvent *event)
{
    TestApp &app = TestApp::instance();

    app.handle_num_.fetch_add(1, std::memory_order_relaxed);
    if (app.push_end_.load(std::memory_order_relaxed)) {
        int handle_num = app.handle_num_.load(std::memory_order_relaxed);
        if (handle_num == app.push_num_.load(std::memory_order_relaxed)) {
            app.push_end_.store(false, std::memory_order_relaxed);
            auto end_timestamp = zrsocket::OSApi::timestamp();
            printf("BizStageHandler::handle_event total_num_times:%ld spend_time:%lld us\n", handle_num, end_timestamp - app.startup_test_timestamp_);
        }
    }

    switch (event->type()) {
        case TestEventType::EVENT_TEST8:
            {
                //const Test8SedaEvent *test8_event = static_cast<const Test8SedaEvent *>(event);
            }
            break;
        case TestEventType::EVENT_TEST16:
            {
                //const Test16SedaEvent * test16_event = static_cast<const Test16SedaEvent *>(event);
            }
            break;
        default:
            break;
    }

    return 0;
}
