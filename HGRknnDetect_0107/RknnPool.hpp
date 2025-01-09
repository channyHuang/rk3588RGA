#ifndef RKNNPOOL_H
#define RKNNPOOL_H

#include <vector>
#include <iostream>
#include <mutex>
#include <queue>
#include <memory>

#include "ThreadPool.hpp"

// rknnModel模型类, inputType模型输入类型, outputType模型输出类型
template <typename rknnModel, typename inputType, typename outputType>
class RknnPool
{
public:
    RknnPool();
    RknnPool(const std::string modelPath, int threadNum);
    int setPathAndNum(const std::string modelPath, int threadNum);
    int init();
    // 模型推理/Model inference
    int put(inputType inputData);
    // 获取推理结果/Get the results of your inference
    int get(outputType &outputData);
    ~RknnPool();

    void deinit();

private:
    int threadNum;
    std::string modelPath;

    long long id;
    // std::mutex idMtx, queueMtx;
    std::unique_ptr<HG::ThreadPool> pool;
    // std::shared_ptr<HG::ThreadPool> pool;
    std::queue<std::future<outputType>> futs;
    std::vector<std::shared_ptr<rknnModel>> models;

    std::mutex *m_pIdMutex, *m_pQueueMutex;

protected:
    int getModelId();
};

template <typename rknnModel, typename inputType, typename outputType>
RknnPool<rknnModel, inputType, outputType>::RknnPool()
{
    this->modelPath = "";
    this->threadNum = 1;
    this->id = 0;

    m_pIdMutex = new std::mutex();
    m_pQueueMutex = new std::mutex();
}

template <typename rknnModel, typename inputType, typename outputType>
RknnPool<rknnModel, inputType, outputType>::RknnPool(const std::string modelPath, int threadNum)
{
    this->modelPath = modelPath;
    this->threadNum = threadNum;
    this->id = 0;

    m_pIdMutex = new std::mutex();
    m_pQueueMutex = new std::mutex();
}

template <typename rknnModel, typename inputType, typename outputType>
int RknnPool<rknnModel, inputType, outputType>::setPathAndNum(const std::string modelPath, int threadNum) {
    this->modelPath = modelPath;
    this->threadNum = threadNum;
    this->id = 0;
}

template <typename rknnModel, typename inputType, typename outputType>
int RknnPool<rknnModel, inputType, outputType>::init()
{
    try
    {
        this->pool = std::make_unique<HG::ThreadPool>(this->threadNum);
        for (int i = 0; i < this->threadNum; i++)
            models.push_back(std::make_shared<rknnModel>(this->modelPath.c_str()));
    }
    catch (const std::bad_alloc &e)
    {
        std::cout << "Out of memory: " << e.what() << std::endl;
        return -1;
    }
    // 初始化模型/Initialize the model
    for (int i = 0, ret = 0; i < threadNum; i++)
    {
        ret = models[i]->init(models[0]->get_ctx(), i != 0);
        if (ret != 0)
            return ret;
    }

    return 0;
}

template <typename rknnModel, typename inputType, typename outputType>
void RknnPool<rknnModel, inputType, outputType>::deinit() {
    for (int i = 0; i < this->threadNum; i++) {
        models[i]->deinit(i != 0);
    }
    models.clear();
    this->pool = nullptr;
}

template <typename rknnModel, typename inputType, typename outputType>
int RknnPool<rknnModel, inputType, outputType>::getModelId()
{
    // std::lock_guard<std::mutex> lock(idMtx);
    std::lock_guard<std::mutex> lock(*m_pIdMutex);
    int modelId = id % threadNum;
    id++;
    return modelId;
}

template <typename rknnModel, typename inputType, typename outputType>
int RknnPool<rknnModel, inputType, outputType>::put(inputType inputData)
{
    // std::lock_guard<std::mutex> lock(queueMtx);
    std::lock_guard<std::mutex> lock(*m_pQueueMutex);
    futs.push(pool->submit(&rknnModel::infer, models[this->getModelId()], inputData));
    return 0;
}

template <typename rknnModel, typename inputType, typename outputType>
int RknnPool<rknnModel, inputType, outputType>::get(outputType &outputData)
{
    // std::lock_guard<std::mutex> lock(queueMtx);
    std::lock_guard<std::mutex> lock(*m_pQueueMutex);
    if(futs.empty() == true)
        return 1;
    outputData = futs.front().get();
    futs.pop();
    return 0;
}

template <typename rknnModel, typename inputType, typename outputType>
RknnPool<rknnModel, inputType, outputType>::~RknnPool()
{
    while (!futs.empty())
    {
        outputType temp = futs.front().get();
        futs.pop();
    }

    delete m_pIdMutex;
    delete m_pQueueMutex;
}

#endif
