#include <Geode/Geode.hpp>
#include <Geode/loader/SettingV3.hpp>
#include <matjson.hpp>

#include <regex>

using namespace geode::prelude;

#include <geode.custom-keybinds/include/Keybinds.hpp>
using namespace keybinds;

inline void registerBinds(matjson::Value keys = {}) {
    keys = keys.size() ? keys : getMod()->getSavedSettingsData()["key-assign-items"];
    std::vector<BindableAction> bindable_actions;
    for (auto& key_value : keys) {
        auto id = key_value.getKey().value_or("unk");

        auto old = BindManager::get()->getBindable(id);
        bindable_actions.push_back(
            {
                id,
                id, //thats all to remove '{\n' and '\n}' vvv
                [](std::string const& s) { return std::string(s.begin() + 2, s.end() - 2); }(key_value.dump()),
                old ? old->getDefaults() : std::vector<geode::Ref<Bind>>{},
                getMod()->getName()
            }
        );

        static std::vector<EventListener<keybinds::InvokeBindFilter>*> listeners;

        listeners.push_back(new EventListener([=](InvokeBindEvent* event) {

            Bind* bind;
            if (key_value.contains("button")) bind = ControllerBind::parse(key_value);
			else if (key_value.contains("key")) bind = Keybind::parse(key_value);
			else return ListenerResult::Propagate;

            PressBindEvent(bind, event->isDown()).post();

            auto disp = CCKeyboardDispatcher::get();

            Modifier mod = static_cast<Modifier>(key_value["mod"].asInt().unwrapOrDefault());
			if (mod == Modifier::Control) disp->dispatchKeyboardMSG(KEY_Control, event->isDown(), 0);
			if (mod == Modifier::Alt) disp->dispatchKeyboardMSG(KEY_Alt, event->isDown(), 0);
			if (mod == Modifier::Shift) disp->dispatchKeyboardMSG(KEY_Shift, event->isDown(), 0);

            auto key = key_value["button"].asInt()
                ? key_value["button"].asInt().unwrapOrDefault()
                : key_value["key"].asInt().unwrapOrDefault();
            disp->dispatchKeyboardMSG(
                static_cast<enumKeyCodes>(key),
                event->isDown(), 0//repeat
            );

            return ListenerResult::Propagate;
            }, InvokeBindFilter(nullptr, id)));
    }
    BindManager::get()->removeCategory(getMod()->getName());
    for (auto& bindable_action : bindable_actions) BindManager::get()->registerBindable(bindable_action);
}

//ya

class BoolSettingNodeV3 : public SettingNodeV3 {};

class KeyAssingItemsSetting : public SettingBaseValueV3<matjson::Value> {
public:

    static Result<std::shared_ptr<SettingV3>> parse(std::string const& key, std::string const& modID, matjson::Value const& json) {
        auto res = std::make_shared<KeyAssingItemsSetting>();
        auto root = checkJson(json, "KeyAssingItemsSetting");
        res->parseBaseProperties(key, modID, root);
        root.checkUnknownKeys();
        return root.ok(std::static_pointer_cast<SettingV3>(res));
    }

    class Node : public SettingValueNodeV3<KeyAssingItemsSetting> {
    public:
        void setupList() {
            auto content = this->getParent();
            if (!content) return log::error(
                "Unable to get content layer, its {} as parent of {}"
                """""""""""""""""""""""""""", this->getParent(), this
            );

            auto oldHeight = content->getContentHeight();

            while (auto item = content->getChildByType<BoolSettingNodeV3>(0)) {
                item->removeFromParent();
            }

            auto keys = this->getValue();
            for (auto& key_value : keys) {
                auto aw = matjson::Value();
                aw["type"] = "bool";
                aw["name"] = key_value.getKey().value_or("unk");
                aw["default"] = false;
                auto setting = BoolSettingV3::parse("dummy", GEODE_MOD_ID, aw);
                if (setting) {
                    auto node = setting.unwrapOrDefault()->createNode(this->getContentWidth());
                    content->addChild(node);
                    //bg
                    node->setDefaultBGColor(ccc4(0, 0, 0, content->getChildrenCount() % 2 == 0 ? 60 : 20));
                    //btn
                    auto btn = node->getButtonMenu()->getChildByType<CCMenuItemToggler>(0);
                    auto icon = CCSprite::createWithSpriteFrameName("GJ_deleteIcon_001.png");
                    limitNodeSize(icon, (btn->m_onButton->getContentSize() - CCSizeMake(14, 14)), 12.f, .1f);
                    btn->m_offButton->addChildAtPosition(icon, Anchor::Center, {}, false);
                    btn->m_onButton->addChildAtPosition(icon, Anchor::Center, {}, false);
                    //action
                    CCMenuItemExt::assignCallback<CCMenuItemToggler>(
                        btn, [__this = Ref(this), key_value](CCMenuItemToggler* sender) {
                            sender->toggle(0);
                            auto list = __this->getValue();
                            list.erase(key_value.getKey().value_or("unk"));
                            __this->setValue(list, __this);
                            __this->updateState(__this);
                        }
                    );
                }
                else log::error("Unable to create setting node: {}", setting.err());
            }
            if (auto layout = typeinfo_cast<ColumnLayout*>(content->getLayout())) {
                layout->setAxisReverse(false);
            };
            this->setZOrder(999);//to be top always
            content->updateLayout();

            if (auto scroll = typeinfo_cast<CCScrollLayerExt*>(content->getParent())) {
                scroll->scrollLayer(oldHeight - content->getContentHeight());
            }
        }
        bool init(std::shared_ptr<KeyAssingItemsSetting> setting, float width) {
            if (!SettingValueNodeV3::init(setting, width)) return false;
            this->setContentHeight(18.f);

            this->getButtonMenu()->addChildAtPosition(CCMenuItemExt::createSpriteExtraWithFrameName(
                "GJ_plus3Btn_001.png", 0.65f, [__this = Ref(this)](CCMenuItem* sender) {

                    sender->getChildByType<CCNode*>(0)->stopActionByTag(658290); //touchMePlsAnim
                    sender->getChildByType<CCNode*>(0)->setScale(0.65f); //bruh ' -'

                    class ListenerContainer : public CCNode {
                    public: EventListener<keybinds::PressBindFilter> m_listener; CREATE_FUNC(ListenerContainer)
                    };
                    static Ref<ListenerContainer> pListenerContainer;
                    if (pListenerContainer) {
                        pListenerContainer->removeFromParent();
                        pListenerContainer = nullptr;
					}
                    pListenerContainer = ListenerContainer::create();

                    static Ref<Bind> bind;

                    static Ref<Notification> ntfy = Notification::create("");
                    ntfy->setID("BindNotification"_spr);
                    if (bind) {
                        __this->getButtonMenu()->removeChildByID("BindSprite"_spr);
                        if (bind) {
                            //log::debug("{} //{}", bind, bind->toString());
                            auto list = __this->getValue();
                            list[bind->toString()] = bind->save();
                            __this->setValue(list, __this);
                            __this->updateState(__this);
                        }
                        bind = nullptr;
                        ntfy->hide();
                        return;
                    }

                    ntfy->setString("Press any keys to send bind selection...");
                    pListenerContainer->m_listener.bind(
                        [__this, sender = Ref(sender)](PressBindEvent* event) mutable -> ListenerResult
                        {
                            if (event->isDown()) {
                                if (event->getBind() == nullptr) return ListenerResult::Stop;

                                __this->getButtonMenu()->removeChildByID("BindSprite"_spr);
                                auto bindSprite = event->getBind()->createBindSprite();
                                bindSprite->setID("BindSprite"_spr);
                                bindSprite->setScale(0.375f);
                                bindSprite->setAnchorPoint(ccp(1.0f, 0.5f));
                                __this->getButtonMenu()->addChildAtPosition(bindSprite, Anchor::Right, ccp(-18.f, 0));

                                bind = event->getBind();
                                ntfy->setString("Click on plus button again to accept selection!");
                                if (sender) {
                                    auto img = sender->getChildByType<CCNode*>(0);
                                    if (img->getActionByTag(658290)) void();
                                    else {
                                        auto touchMePlsAnim = CCRepeatForever::create(CCSequence::create(
                                            CCEaseSineInOut::create(CCScaleTo::create(0.5f, img->getScale() + 0.1f)),
                                            CCEaseSineInOut::create(CCScaleTo::create(0.5f, img->getScale())),
                                            nullptr
                                        ));
                                        touchMePlsAnim->setTag(658290);
                                        sender->getChildByType<CCNode*>(0)->runAction(touchMePlsAnim);
                                    };
                                }
                            }
                            return ListenerResult::Stop;
                        }
                    );
                    ntfy->addChild(pListenerContainer);
                    ntfy->setTime(0.f);
                    ntfy->show();
                }
            ), Anchor::Right, ccp(-6.000, 0));

            return true;
        }
        void updateState(CCNode* invoker) override {
            //log::debug("{}->updateState({})", this, invoker);
            SettingValueNodeV3::updateState(invoker);
            registerBinds(this->getValue());
            getBG()->setOpacity(106);
            setupList();
            if (CCScene::get()) if (auto ntfy = typeinfo_cast<Notification*>(
                CCScene::get()->getChildByID("BindNotification"_spr)
            )) {
                ntfy->hide();
            }
        }
        static Node* create(std::shared_ptr<KeyAssingItemsSetting> setting, float width) {
            auto ret = new Node();
            if (ret && ret->init(setting, width)) {
                ret->autorelease();
                return ret;
            }
            CC_SAFE_DELETE(ret);
            return nullptr;
        }
    };

    SettingNodeV3* createNode(float width) override {
        return Node::create(std::static_pointer_cast<KeyAssingItemsSetting>(shared_from_this()), width);
    };

};

void modLoaded() {
    registerBinds();
    auto ret = Mod::get()->registerCustomSettingType("key-assign-items", &KeyAssingItemsSetting::parse);
    if (!ret) log::error("Unable to register setting type: {}", ret.unwrapErr());
}

$on_mod(Loaded) { modLoaded(); }