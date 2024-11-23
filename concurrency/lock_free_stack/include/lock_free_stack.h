#ifndef CONCURRENCY_LOCK_FREE_STACK_CONTAINERS_INCLUDE_LOCK_FREE_STACK_H
#define CONCURRENCY_LOCK_FREE_STACK_CONTAINERS_INCLUDE_LOCK_FREE_STACK_H

#include <optional>
#include <atomic>
#include <mutex>
#include <vector>
#include <thread>
#include <algorithm>
#include <map>

template <class T>
class HazardPtr;

template <class T>
class Node
{
public:
    Node(const T& key, Node<T> *next = nullptr) : key_(key), next_(next) {}
    
    T key_;
    Node<T> *next_;
}; // class Node

template <class T>
class HazardPtr
{
public:
    std::atomic<Node<T>*> node_ = nullptr;
    std::vector<Node<T>*> del_list_;
    size_t del_count_ = 0;
    size_t thread_num_ = 0;

    HazardPtr(size_t thread_num) : thread_num_(thread_num)
    {
        del_list_.reserve(2 * thread_num);
    }

    HazardPtr() = default;
}; // class HazardPtr


template <class T>
class HazardPtrs
{
public:
    std::map<std::thread::id, HazardPtr<T>*> hps_;
    size_t thread_num_ = 0;

    HazardPtrs(size_t thread_num) : thread_num_(thread_num) {}

    ~HazardPtrs() = default;

    void DelNode(Node<T>* node)
    {
        HazardPtr<T>* my_hp = hps_[std::this_thread::get_id()];
        my_hp->del_list_[my_hp->del_count_++] = node;
        if (my_hp->del_count_ >= thread_num_ * 2)
        {
            Scan();
        }
    }

    void Scan()
    {
        HazardPtr<T>* my_hp = hps_[std::this_thread::get_id()];
        std::vector<Node<T>*> plist;
        for (auto iter : hps_)
        {
            Node<T> *hptr = iter.second->node_.load(std::memory_order_acquire);
            if (hptr != nullptr)
            {
                plist.push_back(hptr);
            }
        }

        std::sort(plist.begin(), plist.end());

        std::vector<Node<T>*> new_del_list;
        size_t new_del_count = 0;

        for (size_t i = 0; i < my_hp->del_count_; ++i)
        {
            if (std::binary_search(plist.begin(), plist.end(), my_hp->del_list_[i]))
            {
                new_del_list[new_del_count++] = my_hp->del_list_[i];
            }
            else
            {
                //std::cerr << "DELETE1\n";
                delete my_hp->del_list_[i];
                //std::cerr << "DELETE2\n";
            }
        }

        for (size_t i = 0; i < new_del_count; i++)
        {
            my_hp->del_list_[i] = new_del_list[i];
        }
        my_hp->del_count_ = new_del_count;
    }
    
}; // class HazardPtrs

template <class T>
class LockFreeStack {

public:
    LockFreeStack(size_t thread_num)
    {
        hps_ = new HazardPtrs<T>{thread_num};
    }

    ~LockFreeStack()
    {
        delete hps_;
    }

    void Push(T val)
    {
        Node<T> *new_node = new Node<T>{val, head_.load(std::memory_order_release)};
        while (!head_.compare_exchange_weak(new_node->next_, new_node, std::memory_order_release))
        {
            ;
        }
    }

    std::optional<T> Pop()
    {
        Node<T> *cur_head = head_.load(std::memory_order_acquire);
        
        auto iter = hps_->hps_.find(std::this_thread::get_id());
        HazardPtr<T>* my_hp = nullptr;
        if (iter == hps_->hps_.end())
        {
            my_hp = new HazardPtr<T>{hps_->thread_num_};
            hps_->hps_.insert(std::make_pair(std::this_thread::get_id(), my_hp));
        }
        else
        {
            my_hp = iter->second;
        }

        do
        {
            if (cur_head == nullptr)
            {
                return std::nullopt;
            }

            my_hp->node_.store(cur_head, std::memory_order_release);

            if (cur_head != head_.load(std::memory_order_acquire)) continue;

        } while (!head_.compare_exchange_weak(cur_head, cur_head->next_, std::memory_order_acquire));
        
        T tmp = cur_head->key_;
        my_hp->node_.store(nullptr, std::memory_order_release);
        hps_->DelNode(cur_head);
        return tmp;
    }

    bool IsEmpty()
    {
        return head_.load(std::memory_order_acquire) == nullptr;
    }

private:
    HazardPtrs<T>* hps_ = nullptr;
    std::atomic<Node<T>*> head_{nullptr};
}; // class LockFreeStack

#endif