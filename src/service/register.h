#ifndef _REGISTER__H_
#define _REGISTER__H_

#include "ulualib.h"

#include "common/common.h"
#include "utils/udict.h"

typedef enum {
  PRIO_LOWEST      = 1, /**< The lowest  priority */
  PRIO_LOW         = 2, /**< A lower than normal  priority */
  PRIO_NORMAL      = 3, /**< The normal  priority */
  PRIO_HIGH        = 4, /**< A higher than normal  priority */
  PRIO_HIGHEST     = 5  /**< The highest  priority */
} task_priority_t;

typedef enum {
  PROP_UNKNOW      = 0,
  PROP_UNACTIVE    = 1,   /**< 未激活 */
  PROP_ONCE        = 2,   /**< 一次性任务*/
  PROP_CYCLE       = 3,   /**< 周期性任务*/
  PROP_PASSIVITY   = 4,   /**< 被动接受 */	
  PROP_DEFER       = 5    /**< 系统退出时执行的任务 **/
} task_property_t;

typedef struct {
  char* alias;  /**< 任务别名 */
  task_priority_t prior;
  task_property_t property; /**< 任务属性 */
  task_property_t oldproperty;
  char* func;
  LUA_HANDLE handle;
} ugTaskType;

extern const char* property_string[];

ugTaskType*  createTaskObj();

void  freeTaskObj(void* );

dict* createTaskMap();

void  destroyTaskMap(dict* p);

int   ugRegister(lua_State* L);

int   ugUnregister(lua_State* L);

int   ugPublish(lua_State* L);

#endif



