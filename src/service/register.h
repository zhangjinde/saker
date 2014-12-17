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
  PROP_UNACTIVE    = 1,   /**< 非激活状态 */
  PROP_ONCE        = 2,   /**< 一次性任务 */
  PROP_CYCLE       = 3,   /**< 循环任务 */
  PROP_PASSIVITY   = 4,   /**< 主动触发 */	
  PROP_DEFER       = 5    /**< 退出时任务 **/
} task_property_t;

typedef struct {
  char *alias;  /**< alias for task */
  task_priority_t prior;
  task_property_t property; /**<  task property @see task_property_t */
  task_property_t oldproperty;
  char *func;
  LUA_HANDLE handle;
} ugTaskType;

extern const char *property_string[];

ugTaskType *createTaskObj(void);

void  freeTaskObj(void *);

dict *createTaskMap(void);

void  destroyTaskMap(dict *p);

int   ugRegister(lua_State *L);

int   ugUnregister(lua_State *L);

int   ugPublish(lua_State *L);

#endif
