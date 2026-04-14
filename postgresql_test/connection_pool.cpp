#pragma once
#include"connection_pool.h"
#include"connection_object.h"

namespace my_project {

    

    Connection_Pool::Connection_Pool():Impl(std::make_unique<CPImpl>()){}

    Connection_Pool::CPImpl::~CPImpl() {
        is_running = false;
        cv.notify_all();
        //std::unique_lock<std::mutex> locker(pool_mutex);
        clearPool();
    }
    Connection_Pool::~Connection_Pool() = default;

    

    bool Connection_Pool::CPImpl::init(const struct SqlPoolParams& params) {

        this->params = params;

        if (params.minSize <= 0) {
            return false;
        }

        for (int i = 0; i < params.minSize; ++i) {
           std::unique_ptr<Connection> conn = std::unique_ptr<Connection>(new Connection(this));
            if (conn != nullptr) {
               
                if (conn->getCImpl()->connect(params.host, params.user, params.passwd,params.port, params.dbname)) {
                    pool.emplace(std::move(conn));
                }
                else {
                    clearPool();
                    return false;
                }
            }
        }

        table_metadata = std::make_unique<my_cache::Cache<std::string,TableMetadata>>(my_cache::CacheType::LRU, 10000);

        std::thread producer(&Connection_Pool::CPImpl::produceConnection, this);
        std::thread recycler(&Connection_Pool::CPImpl::recycleConnection, this);

        producer.detach();
        recycler.detach();
        return true;
    }

    

    void Connection_Pool::CPImpl::clearPool() {
        std::lock_guard<std::mutex> locker(pool_mutex);
        while (!pool.empty()) {
           pool.pop();
        }
    }

    void Connection_Pool::CPImpl::produceConnection() {
        while (is_running.load()) {
            std::unique_lock<std::mutex> locker(pool_mutex);
            while (pool.size() >= params.minSize) {
                cv.wait(locker);
            }
            if(addConnection()==true)
            cv.notify_all();
        }
    }

    void Connection_Pool::CPImpl::recycleConnection() {

        while (is_running.load()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            std::lock_guard<std::mutex> locker(pool_mutex);
            while (pool.size() > params.maxSize &&
                pool.front()->getCImpl()->getAliveTime() >= params.maxIdleTime) {
                auto conn = std::move(pool.front());
                pool.pop();
            }
        }
    }

    bool Connection_Pool::CPImpl::addConnection(){
        auto conn = std::unique_ptr<Connection>(new Connection(this));;
        if (conn == nullptr) return false;
        if(!conn->getCImpl()->connect(params.host, params.user, params.passwd, params.port, params.dbname))
        return false;
        conn->getCImpl()->refreshAliveTime();
        pool.emplace(std::move(conn));
        return true;
    }

    void Connection_Pool::CPImpl::returnConnection(std::unique_ptr<Connection> conn) {
        if (!conn) return;
        std::lock_guard<std::mutex> locker(pool_mutex);
        conn->getCImpl()->refreshAliveTime();
        //printf("?? returnConnection 被调用！\n");
        pool.push(std::move(conn));
        //printf("%d\n",pool.size());
        cv.notify_one();
    }

    bool Connection_Pool::init(const struct SqlPoolParams& params) {
        return Impl->init(params);
    }

    void Connection_Pool::ConnectionDeleter::operator()(Connection* conn) const {
        //printf("?? Deleter 被调用！\n");
        if (conn && impl_) {
            impl_->returnConnection(std::unique_ptr<Connection>(conn));
        }
    }

    std::unique_ptr < Connection, Connection_Pool::ConnectionDeleter> Connection_Pool::CPImpl::getConnection() {
        std::unique_lock<std::mutex> locker(pool_mutex);
        while (pool.empty()) {
            if (std::cv_status::timeout == cv.wait_for(locker, std::chrono::milliseconds(params.timeout))) {
                if (pool.empty()) {
                    continue;
                }
            }
        }
        std::unique_ptr<Connection> conn = std::move(pool.front());
        pool.pop();
        cv.notify_all();
        return std::unique_ptr<Connection, ConnectionDeleter>(conn.release(), ConnectionDeleter(this));
    }

    std::unique_ptr < Connection, Connection_Pool::ConnectionDeleter> Connection_Pool::getConnection() {
        return Impl->getConnection();
    }

    void Connection_Pool::CPImpl::clear_table() {
        table_metadata->clear_all();
    }
    void Connection_Pool::CPImpl::clear_table(const std::string& key) {
        table_metadata->clear(key);
    }
    bool Connection_Pool::CPImpl::put_table(const std::string& key,TableMetadata& value) {
        return table_metadata->put(key, std::move(value));
    }
    auto Connection_Pool::CPImpl::get_table(const std::string& key) {
        return table_metadata->get_shared(key);
    }


}