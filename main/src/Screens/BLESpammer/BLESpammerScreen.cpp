#include "BLESpammerScreen.h"
#include "Devices/Input.h"
#include "Util/Services.h"
#include "Screens/MainMenu/MainMenu.h"
#include "Settings/Settings.h"
#include "LV_Interface/LVGL.h"

BLESpammerScreen::BLESpammerScreen() : queue(4) {
    spammer.init();
	buildUI();
}

void BLESpammerScreen::loop(){
	Event evt{};
	if(queue.get(evt, 0)){
		if(evt.facility == Facility::Input){
			auto eventData = (Input::Data*) evt.data;
			if(eventData->btn == Input::Alt && eventData->action == Input::Data::Press){
				shouldTransition = true;
			}
		}
		free(evt.data);
	}

	statusBar->loop();

	if(shouldTransition){
		transition([](){ return std::make_unique<MainMenu>(); });
		return;
	}
}

void BLESpammerScreen::onStop(){
    spammer.stopSpam();
	Events::unlisten(&queue);
}

void BLESpammerScreen::onStarting(){
    spamSwitch->setValue(false);
}

void BLESpammerScreen::onStart(){
	Events::listen(Facility::Input, &queue);
}

void BLESpammerScreen::updateVisuals(){
	if(bg != nullptr){
		lv_obj_set_style_bg_color(bg, lv_color_black(), 0);
	}
	if(statusBar != nullptr){
		statusBar->updateVisuals();
	}
	if(payloadPicker != nullptr){
		payloadPicker->updateVisuals();
	}
	if(spamSwitch != nullptr){
		spamSwitch->updateVisuals();
	}
	if(exitLabel != nullptr){
		exitLabel->updateVisuals();
	}
}

void BLESpammerScreen::buildUI(){
	lv_obj_set_size(*this, 128, 128);

	bg = lv_obj_create(*this);
	lv_obj_add_flag(bg, LV_OBJ_FLAG_FLOATING);
	lv_obj_set_size(bg, 128, 128);
	lv_obj_set_pos(bg, 0, 0);
	lv_obj_set_style_bg_color(bg, lv_color_black(), 0);
	lv_obj_set_style_bg_opa(bg, LV_OPA_COVER, 0);

	container = lv_obj_create(*this);
	lv_obj_set_size(container, 128, 128 - TopPadding);
	lv_obj_set_pos(container, 0, TopPadding);
	lv_obj_add_flag(container, LV_OBJ_FLAG_SCROLLABLE);
	lv_obj_set_flex_flow(container, LV_FLEX_FLOW_COLUMN);
	lv_obj_set_flex_align(container, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
	lv_obj_set_style_pad_gap(container, 5, 0);
	lv_obj_set_style_pad_hor(container, 1, 0);
	lv_obj_set_style_pad_bottom(container, 1, 0);

	statusBar = new StatusBar(*this);
	lv_obj_add_flag(*statusBar, LV_OBJ_FLAG_FLOATING);
	lv_obj_set_pos(*statusBar, 0, 0);

	payloadPicker = new PickerElement(container, "Target", 0,
									"Apple\nApple Dev\nSamsung\nMicrosoft\nGoogle\nFlipper\nAirTag\nAll",
									[this](uint16_t selected){
                                        selectedType = (BLEPayloadType)selected;
                                        if (spammer.isRunning()) {
                                            spammer.stopSpam();
                                            spammer.startSpam(selectedType);
                                        }
									});
	lv_group_add_obj(inputGroup, *payloadPicker);

	spamSwitch = new BoolElement(container, "Start Spam", [this](bool value){
		if(value){
			spammer.startSpam(selectedType);
		} else {
            spammer.stopSpam();
        }
	}, false);
	lv_group_add_obj(inputGroup, *spamSwitch);

	exitLabel = new LabelElement(container, "Exit", [this](){
		shouldTransition = true;
	}, false, LV_ALIGN_LEFT_MID);
	lv_group_add_obj(inputGroup, *exitLabel);

	for(int i = 0; i < lv_obj_get_child_cnt(container); ++i){
		lv_obj_add_flag(lv_obj_get_child(container, i), LV_OBJ_FLAG_SCROLL_ON_FOCUS);
	}
}
