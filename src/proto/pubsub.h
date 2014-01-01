#ifndef _PUBSUB__H_
#define _PUBSUB__H_

struct ugClient;
struct protoObject;

dict* createPubsubChannels();

list* createPubsubPatterns();

void destroyPubsubChannels(dict* channels);

void destroyPubsubPatterns(list* patterns);

int pubsubSubscribeChannel(struct ugClient *c, struct protoObject * channel) ;

int pubsubUnsubscribeChannel(struct ugClient *c, struct protoObject * channel, int notify);

int pubsubSubscribePattern(struct ugClient *c, struct protoObject * pattern) ;

int pubsubUnsubscribePattern(struct ugClient *c, struct protoObject * pattern, int notify);

int pubsubUnsubscribeAllChannels(struct ugClient *c, int notify) ;

int pubsubUnsubscribeAllPatterns(struct ugClient *c, int notify);

int pubsubPublishMessage(struct protoObject * channel, struct protoObject * message);


void subscribeCommand(struct ugClient *c) ;

void unsubscribeCommand(struct ugClient *c) ;

void psubscribeCommand(struct ugClient *c) ;

void punsubscribeCommand(struct ugClient *c) ;

void publishCommand(struct ugClient *c) ;

void freeAllPubsubObjs();

#endif
