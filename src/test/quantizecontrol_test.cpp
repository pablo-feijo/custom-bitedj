#include "control/controlobject.h"
#include "engine/controls/quantizecontrol.h"
#include "test/mixxxtest.h"

class QuantizeStartupTest : public MixxxTest {};

TEST_F(QuantizeStartupTest, MainDecksStartEnabledDespiteSavedOffAndAllowManualOff) {
    for (const auto& group : {QStringLiteral("[Channel1]"), QStringLiteral("[Channel2]")}) {
        const ConfigKey key(group, QStringLiteral("quantize"));
        config()->setValue(key, 0.0);
        {
            QuantizeControl control(group, config());
            EXPECT_EQ(1.0, ControlObject::get(key));
            ControlObject::set(key, 0.0);
            control.trackLoaded(nullptr);
            EXPECT_EQ(0.0, ControlObject::get(key));
        }
        saveAndReloadConfig();
        QuantizeControl restarted(group, config());
        EXPECT_EQ(1.0, ControlObject::get(key));
    }
}
