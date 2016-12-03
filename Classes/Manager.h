#ifndef __MANAGER_H__
#define __MANAGER_H__


#include "cocos2d.h"
#include <string>
#include <set>

USING_NS_CC;

/*
�Ե���ģʽʵ��Manager
�������������ӵ��������
*/
class  Manager
{
public:

	static Manager * getInstance();
	static void freeInstance(void);

public:
	static Manager * m_manager;
	Manager();
	bool init();

public:
	std::set<std::string> m_list;

};

#endif //