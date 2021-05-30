/*
 * author : Shuichi TAKANO
 * since  : Sat Jan 30 2021 18:16:40
 */
#ifndef _158A32EF_6134_61BA_1152_4B57E8EB5E32
#define _158A32EF_6134_61BA_1152_4B57E8EB5E32

#include <functional>

void startWorker();
void addWorkload(std::function<void()> &&f);

#endif /* _158A32EF_6134_61BA_1152_4B57E8EB5E32 */
