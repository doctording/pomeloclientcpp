#include "ChatRoom.h"
#include "ui/UIEditBox/UIEditBox.h"
#include "ui/UIButton.h"
#include "ui/UIListView.h"
#include "ui/UIText.h"
#include "json\document.h"

USING_NS_CC;
using namespace cocos2d::ui;

#define flag ((void *)0x12)
#define sendFlag ((void *)0x13)


Scene* ChatRoom::createScene(int port, std::string name, std::string roomId)
{
    // 'scene' is an autorelease object
    auto scene = Scene::create();
    
    // 'layer' is an autorelease object
    auto layer = ChatRoom::create();
	layer->scheduleUpdate(); // 更新
	layer->roleName = name;
	layer->roomId = roomId;
	layer->connectToServer(port);

    // add layer as a child to scene
    scene->addChild(layer);

    // return the scene
    return scene;
}

// on "init" you need to initialize your instance
bool ChatRoom::init()
{
	//////////////////////////////
	// 1. super init first
	if (!Layer::init())
	{
		return false;
	}

	Size  visibleSize = Director::getInstance()->getVisibleSize();
	Vec2 origin = Director::getInstance()->getVisibleOrigin();

	isEnter = false;
	usernameList.clear();

// 消息列表
	listView = ListView::create();
	// set list view ex direction
	listView->setDirection(ui::ScrollView::Direction::VERTICAL);
	listView->setBounceEnabled(true);
	listView->setBackGroundImage("green_edit.png");
	listView->setBackGroundImageScale9Enabled(true);
	listView->setContentSize(Size(240, 300));
	listView->setPosition(Vec2(40, 200));
	//listView->setScrollBarPositionFromCorner(Vec2(7, 7));
	this->addChild(listView, 111);

// 在线用户列表
	userlistView = ListView::create();
	userlistView->setDirection(ui::ScrollView::Direction::VERTICAL);
	userlistView->setBounceEnabled(true);
	userlistView->setBackGroundImage("green_edit.png");
	userlistView->setBackGroundImageScale9Enabled(true);
	userlistView->setContentSize(Size(240, 100));
	userlistView->setPosition(Vec2(700, 500));
	this->addChild(userlistView, 111);

// 发送消息的edit框和按钮
	auto editBoxSize = Size(200, 30);
	auto nameEditBox = ui::EditBox::create(editBoxSize, "orange_edit.png");
	nameEditBox->setPosition(Vec2(150, 170));
	this->addChild(nameEditBox);

	auto loginBtn = ui::Button::create("animationbuttonnormal.png","animationbuttonpressed.png");
	loginBtn->setTitleFontSize(20);
	loginBtn->setTitleText("Send Message");
	loginBtn->setAnchorPoint(Vec2(0.5, 0.5));
	loginBtn->setPosition(Vec2(150, 100));
	loginBtn->addClickEventListener([=](Ref*) {
		auto str1 = nameEditBox->getText();
		char str[200];
		sprintf(str, "{\"rid\": \"2\",\"content\":\" %s\",\"from\":\"ccy\",\"target\":\"*\"}", str1);
		pc_request_with_timeout(client, "chat.chatHandler.send", str, sendFlag, 2, request_cb);
	});
	this->addChild(loginBtn, 11);

	return true;
}

void ChatRoom::request_cb(const pc_request_t* req, int rc, const char* resp)
{
	CCLOG("request callback = %s ", resp); // 得到回调的结果 users
	if (resp == NULL)
		return;
#if 1
	using rapidjson::Document;
	Document doc;
	doc.Parse<0>(resp);
	
	if (!doc.HasMember("users")) // 判断json串中是否有此属性
		return;

	int len = doc["users"].Size();
	
	//Manager::getInstance()->m_list.clear();

	std::string all = "";
	
	for(int i=0;i<len;i++)
	{
		std::string s = doc["users"][i].GetString();
		//usernameList.insert(s);
		Manager::getInstance()->m_list.insert(s);
		all += s;
		if(i < len - 1)
			all += ",";
	}
	//CCLOG(all.c_str());
#endif
}

void ChatRoom::event_cb(pc_client_t* client, int ev_type, void* ex_data, const char* arg1, const char* arg2){
	auto chatLayer = (ChatRoom *)ex_data;
	if (ev_type == PC_EV_CONNECTED){
		char reqMsg[200];
		sprintf(reqMsg, "{\"username\": \"%s\",\"rid\":\"%s\"}", chatLayer->roleName.c_str(), chatLayer->roomId.c_str());
		pc_request_with_timeout(client, "connector.entryHandler.enter", reqMsg, (void *)0x14, 2, request_cb);

	}
	else if (ev_type == PC_EV_USER_DEFINED_PUSH){
		// 回调不在主线程，需要先放入队列，在主线程update的时候
		chatLayer->eventMutex.lock();
		chatLayer->eventCbArray.push_back(EventCb{ client, ev_type, ex_data, arg1, arg2 });
		chatLayer->eventMutex.unlock();
		CCLOG("push router = %s, push msg = %s", arg1, arg2);
	}
	CCLOG("event %d =", ev_type);

}

void ChatRoom::connectToServer(int port){
	// pomelo init
	pc_client_config_t config = PC_CLIENT_CONFIG_DEFAULT;
	client = (pc_client_t*)malloc(pc_client_size());
	pc_client_init(client, (void*)0x11, &config);
	handler_id = pc_client_add_ev_handler(client, event_cb, (void *)this, NULL);
	// IP需要替换成服务器所在地址，这里写死掉了
	pc_client_connect(client, "192.168.1.100", port, NULL);
}

void ChatRoom::addMsg(std::string router, std::string msg){
	std::string showStr = "error " + router;
	using rapidjson::Document;
	Document doc;
	doc.Parse<0>(msg.c_str());
	if (router == "onChat"){
		std::string chatMsg = doc["msg"].GetString();
		std::string from = doc["from"].GetString();
		std::string target = doc["target"].GetString();
		showStr = from + " say to " + target + " : " + chatMsg;
	}
	else if (router == "onAdd"){
		std::string user = doc["user"].GetString();
		showStr = "user " + user + " enter the room";

		// 没有 就 增加
		if (usernameList.find(user) == usernameList.end())
			usernameList.insert(user);

		if (Manager::getInstance()->m_list.find(user) == Manager::getInstance()->m_list.end())
			Manager::getInstance()->m_list.insert(user);
	}
	else if (router == "onLeave"){
		std::string user = doc["user"].GetString();
		showStr = "user " + user + " leave the room";

		// 有的话 删除
		if (usernameList.find(user) != usernameList.end())
			usernameList.erase(user);

		if (Manager::getInstance()->m_list.find(user) != Manager::getInstance()->m_list.end())
			Manager::getInstance()->m_list.erase(user);
	}
	auto label = Text::create(showStr, "Arial", 15);
	label->setContentSize(Size(200, 18));
	label->setColor(Color3B::RED);
	listView->insertCustomItem(label,0);

}

void ChatRoom::update(float delta) {

	if ( roleName != "" && isEnter == false)
	{
		auto label = Text::create("welcome:" + roleName, "Arial", 25);
		label->setColor(Color3B::WHITE);
		label->setPosition(Vec2(100, 550));
		this->addChild(label);
		
		// 插入name
		if (usernameList.find(roleName) == usernameList.end())
			usernameList.insert(roleName);

		isEnter = true;
	}
	
	userlistView->removeAllChildrenWithCleanup(true);
	//CCLOG("-------------");
	std::set<std::string>::iterator it = Manager::getInstance()->m_list.begin();//usernameList.begin();
	while (it != Manager::getInstance()->m_list.end())
	{
		std::string str = *it;
		//CCLOG(str.c_str());
		str = "online user:" + str;
		auto label2 = Text::create(str, "Arial", 15);
		label2->setContentSize(Size(200, 18));
		label2->setColor(Color3B::BLUE);
		userlistView->insertCustomItem(label2, 0);

		++it;
	}
	//CCLOG("-------------");

	if (eventCbArray.size() > 0){
		eventMutex.lock();
		for (int i = 0; i < eventCbArray.size(); i++){
			pc_client_t* client = eventCbArray[i].client;
			int ev_type = eventCbArray[i].ev_type;
			void* ex_data = eventCbArray[i].ex_data;
			std::string arg1 = eventCbArray[i].arg1; // 监听的route，返回的消息
			std::string arg2 = eventCbArray[i].arg2;
			CCLOG("chat room %s", arg2);
			addMsg(arg1, arg2); 
		}
		eventCbArray.clear();
		eventMutex.unlock();
	}
}
