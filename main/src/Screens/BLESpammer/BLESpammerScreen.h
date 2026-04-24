#ifndef ARTEMIS_FIRMWARE_BLESPAMMERSCREEN_H
#define ARTEMIS_FIRMWARE_BLESPAMMERSCREEN_H

#include "LV_Interface/LVScreen.h"
#include "BLESpammer.hpp"
#include "Util/Events.h"
#include "UIElements/StatusBar.h"
#include "Screens/Settings/PickerElement.h"
#include "Screens/Settings/LabelElement.h"
#include "Screens/Settings/BoolElement.h"

class BLESpammerScreen : public LVScreen {
public:
	BLESpammerScreen();

private:
	virtual void onStarting() override;
	virtual void onStart() override;
	virtual void onStop() override;
	virtual void loop() override;
	virtual void updateVisuals() override;

	void buildUI();

	lv_obj_t* bg = nullptr;
	lv_obj_t* container = nullptr;
	StatusBar* statusBar = nullptr;
    
    PickerElement* payloadPicker = nullptr;
    BoolElement* spamSwitch = nullptr;
	LabelElement* exitLabel = nullptr;

	bool shouldTransition = false;
	static constexpr uint8_t TopPadding = 13;

	EventQueue queue;
    
    BLESpammer spammer;
    BLEPayloadType selectedType = BLEPayloadType::APPLE_ACTION;
};

#endif //ARTEMIS_FIRMWARE_BLESPAMMERSCREEN_H
