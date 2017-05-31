#include<iostream>  
#include<map>  
#include<string>  
#include<vector>  
#include<utility>  
#include<set>  
#include<deque>  
#include<algorithm>  
#include<boost/any.hpp>  
#include<boost/enable_shared_from_this.hpp>  
#include<boost/noncopyable.hpp>  
#include<boost/scoped_ptr.hpp>  
#include<boost/shared_ptr.hpp>  
#include<boost/weak_ptr.hpp>  
#include<boost/function.hpp>  
#include<boost/static_assert.hpp>  
#include<boost/bind.hpp>  
#include<boost/foreach.hpp>  
#include<boost/ptr_container/ptr_vector.hpp>  
#include<errno.h>  
#include<fcntl.h>  
#include<stdio.h>  
#include<strings.h>  
#include<unistd.h>  
#include<endian.h>  
#include<assert.h>  
#include<stdio.h>  
#include<stdlib.h>  
#include<string.h>  
#include<pthread.h>  
#include<unistd.h>  
#include<poll.h>  
#include<errno.h>  
#include<signal.h>  
#include<stdint.h>  
#include<arpa/inet.h>  
#include<netinet/tcp.h>  
#include<netinet/in.h>  
#include<sys/timerfd.h>  
#include<sys/syscall.h>  
#include<sys/time.h>  
#include<sys/eventfd.h>  
#include<sys/types.h>  
#include<sys/socket.h>  
#include<sys/epoll.h>  
using namespace std;  
using namespace boost;  
# define UINTPTR_MAX       (4294967295U)//һ���޷��Ŵ���  
/* 
*������ 
*/  
class Mutex:noncopyable{  
    public:  
        Mutex(){  
            pthread_mutex_init(&mutex,NULL);  
        }  
        void lock(){  
            pthread_mutex_lock(&mutex);  
        }  
        void unlock(){  
            pthread_mutex_unlock(&mutex);  
        }  
        pthread_mutex_t& get(){  
            return mutex;  
        }  
    private:  
        pthread_mutex_t mutex;  
};  
/* 
*������RAII 
*/  
class MutexLockGuard:noncopyable{  
    public:  
        explicit MutexLockGuard(Mutex& mutex):mutex_(mutex){  
            mutex_.lock();  
        }  
        ~MutexLockGuard(){  
            mutex_.unlock();  
        }  
    private:  
        Mutex& mutex_;  
};  
/* 
*�������� 
*/  
class Condition:noncopyable{  
    public:  
        explicit Condition(Mutex& mutex):mutex_(mutex){  
            pthread_cond_init(&pcond_,NULL);  
        }  
        ~Condition(){  
            pthread_cond_destroy(&pcond_);  
        }  
        void wait(){  
            pthread_cond_wait(&pcond_,&mutex_.get());  
        }  
        void notify(){  
            pthread_cond_signal(&pcond_);  
        }  
        void notifyALL(){  
            pthread_cond_broadcast(&pcond_);  
        }  
    private:  
        Mutex& mutex_;  
        pthread_cond_t pcond_;  
};  
/* 
*����ʱ�� 
*/  
class CountDownLatch{  
    public:  
        CountDownLatch(int count):mutex_(),condition_(mutex_),count_(count){}  
        void wait(){  
            MutexLockGuard lock(mutex_);  
            while(count_>0)  
                condition_.wait();  
        }  
        void countDown(){  
            MutexLockGuard lock(mutex_);  
            --count_;  
            if(count_==0)  
                condition_.notifyALL();  
        }  
    private:  
        mutable Mutex mutex_;  
        Condition condition_;  
        int count_;  
};  
/* 
 *�߳���Thread 
 */  
__thread pid_t t_cacheTid=0;//�߳�˽�������߳�ID����ͨ��ϵͳ���û��ID  
class Thread:noncopyable{  
    public:  
        typedef function<void()> ThreadFunc;//�߳���Ҫִ�й�������  
        explicit Thread(const ThreadFunc& a,const string& name=string()):started_(false),  
            joinded_(false),pthreadID_(0),tid_(new pid_t(0)),func_(a),name_(name){  
            }  
        ~Thread(){  
            if(started_&&!joinded_){  
                pthread_detach(pthreadID_);//�����߳�  
            }  
        }  
        void start();  
        /* 
        { 
            assert(!started_); 
            started_=true; 
            if(pthread_create(&pthreadID_,NULL,&startThread,NULL)){ 
                started_=false; 
                abort();//��ֹ����ˢ�»����� 
            } 
        } 
        *///###1###ʹ�ô˴���������http://cboard.cprogramming.com/cplusplus-programming/113981-passing-class-member-function-pthread_create.html  
        void join(){//�ȴ��߳�ִ���깤������  
            assert(started_);  
            assert(!joinded_);  
            joinded_=true;  
            pthread_join(pthreadID_,NULL);  
        }  
        pid_t tid() const{  
            if(t_cacheTid==0){//���û�л���t_cacheTid���ȡ�߳�ID����ֱ��ͨ���߳�˽�����ݷ���ID����ϵͳ����  
                t_cacheTid=syscall(SYS_gettid);  
            }  
            return t_cacheTid;  
        }  
        const string& name() const{  
            return name_;  
        }  
        //void* startThread(void* arg){//###1###  
        void startThread(){  
            func_();  
        }  
    private:  
        bool started_;  
        bool joinded_;  
        pthread_t pthreadID_;  
        shared_ptr<pid_t> tid_;  
        ThreadFunc func_;  
        string name_;  
};  
void* threadFun(void* arg){//���ü�Ӳ�ִ�й�������  
    Thread* thread=static_cast<Thread*>(arg);  
    thread->startThread();  
    return NULL;  
}  
void Thread::start(){  
    assert(!started_);  
    started_=true;  
    if(pthread_create(&pthreadID_,NULL,threadFun,this)){  
        started_=false;  
        abort();//��ֹ����ˢ�»�����  
    }  
}  
  
/* 
 * �ֲ߳̾�����TSD 
 */  
template<typename T>  
class ThreadLocal:noncopyable{  
    public:  
        ThreadLocal(){  
            pthread_key_create(&pkey_,&destructor);//ÿ���̻߳��趨�Լ���pkey_����pthread_key_deleteִ��destructor����  
        }  
        ~ThreadLocal(){  
            pthread_key_delete(pkey_);//ִ��destructor����  
        }  
        T& value(){//���õ���ģʽ���˴�������߳�ʹ�ùʲ����ڷ��̰߳�ȫ��singleton����  
            T* perThreadValue=static_cast<T*>(pthread_getspecific(pkey_));  
            if(!perThreadValue){  
                T* newObj=new T();  
                pthread_setspecific(pkey_,newObj);  
                perThreadValue=newObj;  
            }  
            return *perThreadValue;  
        }  
    private:  
        static void destructor(void* x){//���˽������  
            T* obj=static_cast<T*>(x);  
            delete obj;  
        }  
    private:  
        pthread_key_t pkey_;  
};  
/* 
 * �̳߳� 
 */  
class ThreadPool:noncopyable{  
    public:  
        typedef function<void()> Task;//�̹߳�������  
        explicit ThreadPool(const string& name=string()):mutex_(),cond_(mutex_),name_(name),running_(false){  
        }  
        ~ThreadPool(){  
            if(running_){  
                stop();//�ȴ������̳߳��е��߳���ɹ���  
            }  
        }  
        void start(int numThreads){  
            assert(threads_.empty());  
            running_=true;  
            threads_.reserve(numThreads);  
            for(int i=0;i<numThreads;i++){  
                threads_.push_back(new Thread(bind(&ThreadPool::runInThread,this)));//�����߳�����runInThread��������  
                threads_[i].start();  
            }  
        }  
        void stop(){  
            running_=false;//��������ʹ���߲�Ҫ�ڴ˺���������ˣ���Ϊֹͣ�ص��ǳػ�Ҫ�ȴ������߳��������  
            cond_.notifyALL();//���ѳ�������˯�ߵ��߳�  
            for_each(threads_.begin(),threads_.end(),bind(&Thread::join,_1));//�ȴ������߳����  
        }  
        void run(const Task& task){  
            if(running_){//###4###��ֹֹͣ�����к�������ӽ���  
                if(threads_.empty()){//����û���߳�  
                    task();  
                }  
                else{  
                    MutexLockGuard guard(mutex_);//ʹ��RAII mutex��֤�̰߳�ȫ  
                    queue_.push_back(task);  
                    cond_.notify();  
                }  
            }  
            else{  
                printf("�̳߳���ֹͣ����\n");  
            }  
        }  
        bool running(){//ʹ���߿��Ի�ȡ�̳߳ص�����״̬  
            return running_;  
        }  
    private:  
        void runInThread(){//�̹߳�������  
            while(running_){//###2###  
                Task task(take());  
                if(task){//task���������ΪNULL  
                    task();  
                }  
            }  
        }  
        Task take(){  
            MutexLockGuard guard(mutex_);  
            while(queue_.empty()&&running_){//###3###��###2###���ܱ�֤�ڳ�ֹͣ���е����̻߳�û����ɲ����ڼ䰲ȫ��������ڼ���������ӵ����У���ĳ���߳�Aִ�е�###2###�����ϱ��л��ˣ���running_=falseֹͣ���У�A���л�������ִ��###3###�������尡����Ϊ���Ѿ�ֹͣ�����ˡ�����###4###���б�Ҫ����ʹ���߳�ֹͣ��һ�龰  
                cond_.wait();//����û������ȴ�  
            }  
            Task task;  
            if(!queue_.empty()){  
                task=queue_.front();  
                queue_.pop_front();  
            }  
            return task;  
        }  
        Mutex mutex_;  
        Condition cond_;  
        string name_;  
        ptr_vector<Thread> threads_;//����ָ������  
        deque<Task> queue_;  
        bool running_;  
};  
/* 
 * ԭ������ 
 */  
template<typename T>  
class AtomicIntegerT : boost::noncopyable  
{  
    public:  
        AtomicIntegerT()  
            : value_(0){}  
        T get() const  
        {  
            return __sync_val_compare_and_swap(const_cast<volatile T*>(&value_), 0, 0);  
        }  
        T getAndAdd(T x)  
        {  
            return __sync_fetch_and_add(&value_, x);  
        }  
        T addAndGet(T x)  
        {  
            return getAndAdd(x) + x;  
        }  
        T incrementAndGet()  
        {  
            return addAndGet(1);  
        }  
        void add(T x)  
        {  
            getAndAdd(x);  
        }  
        void increment()  
        {  
            incrementAndGet();  
        }  
        void decrement()  
        {  
            getAndAdd(-1);  
        }  
        T getAndSet(T newValue)  
        {  
            return __sync_lock_test_and_set(&value_, newValue);  
        }  
    private:  
        volatile T value_;  
};  
typedef AtomicIntegerT<int32_t> AtomicInt32;  
typedef AtomicIntegerT<int64_t> AtomicInt64;  
  
class Channel;//ǰ���������¼��ַ�����Ҫ�����¼�ע�����¼�����(�¼��ص�)  
class Poller;//IO���û��ƣ���Ҫ�����Ǽ����¼����ϣ���select��poll,epoll�Ĺ���  
class Timer;  
class TimerId;  
class Timestamp;  
class TimerQueue;  
class TcpConnection;  
class Buffer;  
  
typedef shared_ptr<TcpConnection> TcpConnectionPtr;  
typedef function<void()> TimerCallback;  
typedef function<void (const TcpConnectionPtr&)> ConnectionCallback;  
typedef function<void (const TcpConnectionPtr&,Buffer* buf)> MessageCallback;  
typedef function<void (const TcpConnectionPtr&)> WriteCompleteCallback;  
typedef function<void (const TcpConnectionPtr&)> CloseCallback;  
/* 
*EventLoop: �¼�ѭ����һ���߳�һ���¼�ѭ����one loop per thread������Ҫ�����������¼�ѭ����ȴ��¼�����Ȼ���������¼� 
*/  
class EventLoop:noncopyable{  
    public:  
        //ʵ���¼�ѭ��  
        //ʵ�ֶ�ʱ�ص����ܣ�ͨ��timerfd��TimerQueueʵ��  
        //ʵ���û�����ص���Ϊ���̰߳�ȫ�п��������߳���IO�̵߳�EventLoop������񣬴�ʱͨ��eventfd֪ͨEventLoopִ���û�����  
        typedef function<void()> Functor;//�ص�����  
        EventLoop();  
        ~EventLoop();  
        void loop();//EventLoop������,�����¼�ѭ����Eventloop::loop()->Poller::Poll()��þ������¼����ϲ�ͨ��Channel::handleEvent()ִ�о����¼��ص�  
        void quit();//��ֹ�¼�ѭ����ͨ���趨��־λ������һ���ӳ�  
        //Timestamp pollReturnTime() const;  
        void assertInLoopThread(){//�������̲߳�ӵ��EventLoop���˳�����֤one loop per thread  
            if(!isInLoopThread()){  
                abortNotInLoopThread();  
            }  
        }  
        bool isInLoopThread() const{return threadID_==syscall(SYS_gettid);}//�ж������߳��Ƿ�Ϊӵ�д�EventLoop���߳�  
        TimerId runAt(const Timestamp& time,const TimerCallback& cb);//����ʱ��ִ�ж�ʱ���ص�cb  
        TimerId runAfter(double delay,const TimerCallback& cb);//���ʱ��ִ�ж�ʱ���ص�  
        TimerId runEvery(double interval,const TimerCallback& cb);//ÿ��intervalִ�ж�ʱ���ص�  
        void runInLoop(const Functor& cb);//����IO�߳�ִ���û��ص�(��EventLoop����ִ���¼��ص������ˣ���ʱ�û�ϣ������EventLoopִ���û�ָ��������)  
        void queueInLoop(const Functor& cb);//����IO�߳�(ӵ�д�EventLoop���߳�)�����û�ָ��������ص��������  
        void cancel(TimerId tiemrId);  
        void wakeup();//����IO�߳�  
        void updateChannel(Channel* channel);//�����¼��ַ���Channel������ļ�������fd���¼�����ע���¼����¼��ص�����  
        void removeChannel(Channel* channel);  
    private:  
        void abortNotInLoopThread();//�ڲ�ӵ��EventLoop�߳�����ֹ  
        void handleRead();//timerfd�Ͽɶ��¼��ص�  
        void doPendingFunctors();//ִ�ж���pendingFunctors�е��û�����ص�  
        typedef vector<Channel*> ChannelList;//�¼��ַ���Channel������һ��Channelֻ����һ���ļ�������fd���¼��ַ�  
        bool looping_;//�¼�ѭ������loop�����б�־  
        bool quit_;//ȡ��ѭ�������־  
        const pid_t threadID_;//EventLoop�ĸ����߳�ID  
        scoped_ptr<Poller> poller_;//IO������Poller���ڼ����¼�����  
        //scoped_ptr<Epoller> poller_;  
        ChannelList activeChannels_;//������poll�ľ����¼����ϣ����Ｏ�ϻ���Channel(�¼��ַ����߱������¼��ص�����)  
        //Timestamp pollReturnTime_;  
        int wakeupFd_;//eventfd���ڻ���EventLoop�����߳�  
        scoped_ptr<Channel> wakeupChannel_;//ͨ��wakeupChannel_�۲�wakeupFd_�ϵĿɶ��¼������ɶ�ʱ������Ҫ����EventLoop�����߳�ִ���û��ص�  
        Mutex mutex_;//���������Ա�������  
        vector<Functor> pendingFunctors_;//�û�����ص�����  
        scoped_ptr<TimerQueue> timerQueue_;//��ʱ���������ڴ�Ŷ�ʱ��  
        bool callingPendingFunctors_;//�Ƿ����û�����ص���־  
};  
  
/* 
 *Poller: IO Multiplexing Poller��poll�ķ�װ����Ҫ����¼����ϵļ��� 
 */  
class Poller:noncopyable{//�����ں�EventLoopһ��������ӵ��Channel  
    public:  
        typedef vector<Channel*> ChannelList;//Channel����(Channel�������ļ�������fd��fdע����¼����¼��ص�����)��Channel�����ļ�����������ע���¼������¼��ص�������������Ҫ���ڷ��ؾ����¼�����  
        Poller(EventLoop* loop);  
        ~Poller();  
        Timestamp Poll(int timeoutMs,ChannelList* activeChannels);//Poller�ĺ��Ĺ��ܣ�ͨ��pollϵͳ���ý������¼�����ͨ��activeChannels���أ���EventLoop::loop()->Channel::handelEvent()ִ����Ӧ�ľ����¼��ص�  
        void updateChannel(Channel* channel);//Channel::update(this)->EventLoop::updateChannel(Channel*)->Poller::updateChannel(Channel*)����ά���͸���pollfs_��channels_,���»����Channel��Poller��pollfds_��channels_��(��Ҫ���ļ�������fd��Ӧ��Channel�������޸��Ѿ���pollע����¼�����fd����pollע���¼�)  
        void assertInLoopThread(){//�ж��Ƿ��EventLoop��������ϵ��EventLoopҪӵ�д�Poller  
            ownerLoop_->assertInLoopThread();  
        }  
        void removeChannel(Channel* channel);//ͨ��EventLoop::removeChannel(Channel*)->Poller::removeChannle(Channel*)ע��pollfds_��channels_�е�Channel  
    private:  
        void fillActiveChannels(int numEvents,ChannelList* activeChannels) const;//����pollfds_�ҳ������¼���fd����activeChannls,���ﲻ��һ�߱���pollfds_һ��ִ��Channel::handleEvent()��Ϊ���߿�����ӻ���ɾ��Poller�к�Channel��pollfds_��channels_(����������ͬʱ�����������ܱ��޸���Σ�յ�),����Poller�����Ǹ���IO���ã��������¼��ַ�(����Channel����)  
        typedef vector<struct pollfd> PollFdList;//struct pollfd��pollϵͳ���ü������¼����ϲ���  
        typedef map<int,Channel*> ChannelMap;//�ļ�������fd��IO�ַ���Channel��ӳ�䣬ͨ��fd���Կ����ҵ�Channel  
        //ע��:Channel����fd��Ա�������Channelӳ�䵽fd�Ĺ��ܣ�����fd��Channel�������˫��  
        EventLoop* ownerLoop_;//������EventLoop  
        PollFdList pollfds_;//�����¼�����  
        ChannelMap channels_;//�ļ�������fd��Channel��ӳ��  
};  
  
/* 
 *Channel: �¼��ַ���,����������ļ�������fd��fd���������¼����¼��Ĵ�����(�¼��ص�����) 
 */  
class Channel:noncopyable{  
    public:  
        typedef function<void()> EventCallback;//�¼��ص���������,�ص������Ĳ���Ϊ�գ����ｫ���������Ѿ�д����  
        typedef function<void()> ReadEventCallback;  
        Channel(EventLoop* loop,int fd);//һ��Channelֻ����һ���ļ�������fd��Channel��ӵ��fd���ɼ��ṹӦ���������ģ�EventLoop����Poller�����¼����ϣ��������¼�����Ԫ�ؾ���Channel����Channel�Ĺ��ܲ����Ƿ��ؾ����¼������߱��¼�������  
        ~Channel();//Ŀǰȱʧһ�����ܣ�~Channel()->EventLoop::removeChannel()->Poller::removeChannel()ע��Poller::map<int,Channel*>��Channel*�������ָ��  
        void handleEvent();//����Channel�ĺ��ģ���fd��Ӧ���¼�������Channel::handleEvent()ִ����Ӧ���¼��ص�����ɶ��¼�ִ��readCallback_()  
        void setReadCallback(const ReadEventCallback& cb){//�ɶ��¼��ص�  
            readCallback_=cb;  
        }  
        void setWriteCallback(const EventCallback& cb){//��д�¼��ص�  
            writeCallback_=cb;  
        }  
        void setErrorCallback(const EventCallback& cb){//�����¼��ص�  
            errorCallback_=cb;  
        }  
        void setCloseCallback(const EventCallback& cb){  
            closeCallback_=cb;  
        }  
        int fd() const{return fd_;}//����Channel������ļ�������fd��������Channel��fd��ӳ��  
        int events() const{return events_;}//����fd��ע����¼�����  
        void set_revents(int revt){//�趨fd�ľ����¼����ͣ���poll���ؾ����¼��󽫾����¼����ʹ����˺�����Ȼ��˺�������handleEvent��handleEvent���ݾ����¼������;���ִ���ĸ��¼��ص�����  
            revents_=revt;  
        }  
        bool isNoneEvent() const{//fdû����Ҫע����¼�  
            return events_==kNoneEvent;  
        }  
        void enableReading(){//fdע��ɶ��¼�  
            events_|=kReadEvent;  
            update();  
        }  
        void enableWriting(){//fdע���д�¼�  
            events_|=kWriteEvent;  
            update();  
        }  
        void disableWriting(){  
            events_&=~kWriteEvent;  
            update();  
        }  
        void disableAll(){events_=kReadEvent;update();}  
        bool isWriting() const{  
            return events_&kWriteEvent;  
        }  
        int index(){return index_;}//index_�Ǳ�Channel�����fd��poll�����¼����ϵ��±꣬���ڿ���������fd��pollfd  
        void set_index(int idx){index_=idx;}  
        EventLoop* ownerLoop(){return loop_;}  
    private:  
        void update();//Channel::update(this)->EventLoop::updateChannel(Channel*)->Poller::updateChannel(Channel*)���Poller�޸�Channel����Channel�Ѿ�������Poller��vector<pollfd> pollfds_(����Channel::index_��vector���±�)�����ChannelҪ����ע���¼���Poller����Channel::events()����¼�������vector�е�pollfd;��Channelû����vector������Poller��vector����µ��ļ��������¼����¼����У�����vector.size(),(vectorÿ�����׷��)����Channel::set_index()��ΪChannel��ס�Լ���Poller�е�λ��  
        static const int kNoneEvent;//���κ��¼�  
        static const int kReadEvent;//�ɶ��¼�  
        static const int kWriteEvent;//��д�¼�  
        bool eventHandling_;  
        EventLoop* loop_;//Channel������EventLoop(ԭ����EventLoop��Poller��Channel����һ��IO�߳�)  
        const int fd_;//ÿ��ChannelΨһ������ļ���������Channel��ӵ��fd  
        int events_;//fd_ע����¼�  
        int revents_;//ͨ��poll���صľ����¼�����  
        int index_;//��poll�ļ����¼�����pollfd���±꣬���ڿ���������fd��pollfd  
        ReadEventCallback readCallback_;//�ɶ��¼��ص���������poll����fd_�Ŀɶ��¼�ʱ���ô˺���ִ����Ӧ���¼������ú������û�ָ��  
        EventCallback writeCallback_;//��д�¼��ص�����  
        EventCallback errorCallback_;//�����¼��ص�����  
        EventCallback closeCallback_;  
};  
  
/* 
*ʱ���������һ��������ʾ΢���� 
*/  
class Timestamp{  
    public:  
        Timestamp():microSecondsSinceEpoch_(0){}  
        explicit Timestamp(int64_t microseconds):microSecondsSinceEpoch_(microseconds){}  
        void swap(Timestamp& that){  
            std::swap(microSecondsSinceEpoch_,that.microSecondsSinceEpoch_);  
        }  
        bool valid() const{return microSecondsSinceEpoch_>0;}  
        int64_t microSecondsSinceEpoch() const {return microSecondsSinceEpoch_;}  
        static Timestamp now(){  
            struct timeval tv;  
            gettimeofday(&tv, NULL);  
            int64_t seconds = tv.tv_sec;  
            return Timestamp(seconds * kMicroSecondsPerSecond + tv.tv_usec);  
        }  
        static Timestamp invalid(){return Timestamp();}  
        static const int kMicroSecondsPerSecond=1000*1000;  
    private:  
        int64_t microSecondsSinceEpoch_;  
};  
//ʱ����ıȽ�  
inline bool operator<(Timestamp lhs, Timestamp rhs)  
{  
  return lhs.microSecondsSinceEpoch() < rhs.microSecondsSinceEpoch();  
}  
  
inline bool operator==(Timestamp lhs, Timestamp rhs)  
{  
  return lhs.microSecondsSinceEpoch() == rhs.microSecondsSinceEpoch();  
}  
inline double timeDifference(Timestamp high, Timestamp low)  
{  
  int64_t diff = high.microSecondsSinceEpoch() - low.microSecondsSinceEpoch();  
  return static_cast<double>(diff) / Timestamp::kMicroSecondsPerSecond;  
}  
inline Timestamp addTime(Timestamp timestamp, double seconds)  
{  
  int64_t delta = static_cast<int64_t>(seconds * Timestamp::kMicroSecondsPerSecond);  
  return Timestamp(timestamp.microSecondsSinceEpoch() + delta);  
}  
/* 
 * TimerId����Ψһ��ŵ�Timer 
 */  
class TimerId{  
    public:  
        TimerId(Timer* timer=NULL,int64_t seq=0)  
            :timer_(timer),sequence_(seq){}  
        friend class TimerQueue;  
    private:  
        Timer* timer_;  
        int64_t sequence_;  
};  
/* 
 *��ʱ�� 
 */  
class Timer : boost::noncopyable  
{  
    public:  
        typedef function<void()> TimerCallback;//��ʱ���ص�����  
        //typedef function<void()> callback;  
        Timer(const TimerCallback& cb, Timestamp when, double interval)  
            :callback_(cb),expiration_(when),  
            interval_(interval),repeat_(interval > 0.0),  
            sequence_(s_numCreated_.incrementAndGet()){}  
        void run() const {//ִ�ж�ʱ���ص�  
            callback_();  
        }  
        Timestamp expiration() const  { return expiration_; }//���ض�ʱ���ĳ�ʱʱ���  
        bool repeat() const { return repeat_; }//�Ƿ������Զ�ʱ  
        int64_t sequence() const{return sequence_;}  
        void restart(Timestamp now);//���ö�ʱ��  
     private:  
        const TimerCallback callback_;//��ʱ�ص�����  
        Timestamp expiration_;//��ʱʱ���  
        const double interval_;//���ʱ�䣬��Ϊ��������ʱ������ɾ���ĳ�ʱʱ��  
        const bool repeat_;//�Ƿ��ظ���ʱ��־  
        const int64_t sequence_;//  
        static AtomicInt64 s_numCreated_;//ԭ�Ӳ������������ɶ�ʱ��ID  
};  
AtomicInt64 Timer::s_numCreated_;  
void Timer::restart(Timestamp now){  
    if (repeat_){//���ڶ�ʱ  
        expiration_ = addTime(now, interval_);  
    }  
    else{  
        expiration_ = Timestamp::invalid();  
    }  
}  
/* 
 * timerfd����ز���,������TimerQueueʵ�ֳ�ʱ������ 
 */  
int createTimerfd(){//����timerfd  
    int timerfd = ::timerfd_create(CLOCK_MONOTONIC,TFD_NONBLOCK | TFD_CLOEXEC);  
    if (timerfd < 0){  
        printf("Timderfd::create() error\n");  
    }  
    return timerfd;  
}  
struct timespec howMuchTimeFromNow(Timestamp when){  
    int64_t microseconds = when.microSecondsSinceEpoch()- Timestamp::now().microSecondsSinceEpoch();  
    if (microseconds < 100){  
        microseconds = 100;  
    }  
    struct timespec ts;  
    ts.tv_sec = static_cast<time_t>(microseconds / Timestamp::kMicroSecondsPerSecond);  
    ts.tv_nsec = static_cast<long>((microseconds % Timestamp::kMicroSecondsPerSecond) * 1000);  
    return ts;  
}  
void readTimerfd(int timerfd, Timestamp now){//timerfd�Ŀɶ��¼��ص�  
    uint64_t howmany;  
    ssize_t n = ::read(timerfd, &howmany, sizeof howmany);  
    if (n != sizeof howmany){  
        printf("readTimerfd error\n");  
    }  
}  
void resetTimerfd(int timerfd, Timestamp expiration)//����timerfd�ļ�ʱ  
{  
    struct itimerspec newValue;  
    struct itimerspec oldValue;  
    bzero(&newValue, sizeof newValue);  
    bzero(&oldValue, sizeof oldValue);  
    newValue.it_value = howMuchTimeFromNow(expiration);  
    int ret = ::timerfd_settime(timerfd, 0, &newValue, &oldValue);  
    if (ret){  
        printf("timerfd_settime erro\n");  
    }  
}  
/* 
 *��ʱ������ 
 */  
class TimerQueue : boost::noncopyable  
{//��ͨ�����һ��timerfd��EventLoop�У���timerfd�ɶ��¼�����ʱ��TimerQueue::handleRead()���������ڵĳ�ʱ�Ķ�ʱ����ִ����Щ��ʱ��ʱ���Ļص�  
 //��ʱ������Ϊset<pair<Timestamp,Timer*> >������pair��Ϊkey��ԭ���ǿ�����һ��ʱ���ж����ͬ��Timestampʱ���  
 //����ʱ������set��Ӷ�ʱ��timer��ʱ����鵱ǰ��С�Ķ�ʱ����������С�Ķ�ʱʱ�丶����timerfd  
    public:  
        typedef function<void()> TimerCallback;//��ʱ���ص�  
        TimerQueue(EventLoop* loop);  
        ~TimerQueue();  
        TimerId addTimer(const TimerCallback& cb,Timestamp when,double interval);//��Ӷ�ʱ������ʱ��������  
        void cancel(TimerId timerId);  
    private:  
        typedef pair<Timestamp, Timer*> Entry;//���ô���Ϊ��ֵ  
        typedef set<Entry> TimerList;//setֻ��key��value������  
        typedef pair<Timer*,int64_t> ActiveTimer;  
        typedef set<ActiveTimer> ActiveTimerSet;  
  
        void handleRead();//timerfd�Ŀɶ��ص�  
        void addTimerInLoop(Timer* timer);//��Ӷ�ʱ��  
        void cancelInLoop(TimerId timerId);  
        std::vector<Entry> getExpired(Timestamp now);//��ȡ���г�ʱ�Ķ�ʱ��  
        void reset(const std::vector<Entry>& expired, Timestamp now);//��ʱ�Ķ�ʱ���Ƿ���Ҫ���¶�ʱ  
        bool insert(Timer* timer);//���붨ʱ����������  
  
        EventLoop* loop_;//TimerQueue������EventLoop  
        const int timerfd_;//��ʱ�����б�����Ҫ�ڶ�ʱ����ʱ��ִ�ж��������г�ʱ��ʱ���Ļص�  
        Channel timerfdChannel_;//����timerfdChannel_�۲�timerfd_�Ŀɶ��¼�������timerfd_�ɶ�������ʱ���������ж�ʱ����ʱ  
        TimerList timers_;//��ʱ������  
        bool callingExpiredTimers_;  
        ActiveTimerSet activeTimers_;  
        ActiveTimerSet cancelingTimers_;  
};  
/* 
 * TimerQueueʵ�� 
 */  
TimerQueue::TimerQueue(EventLoop* loop)  
    :loop_(loop),timerfd_(createTimerfd()),  
    timerfdChannel_(loop, timerfd_),timers_(),  
    callingExpiredTimers_(false)  
{  
    timerfdChannel_.setReadCallback(bind(&TimerQueue::handleRead, this));  
    timerfdChannel_.enableReading();//timerfdע��ɶ��¼�  
}  
TimerQueue::~TimerQueue(){  
    ::close(timerfd_);  
    for (TimerList::iterator it = timers_.begin();it != timers_.end(); ++it)  
    {  
        delete it->second;  
    }  
}  
TimerId TimerQueue::addTimer(const TimerCallback& cb,Timestamp when,double interval)//�����߳���IO�߳�����û��ص�ʱ����Ӳ���ת�Ƶ�IO�߳���ȥ���Ӷ���֤�̰߳�ȫone loop per thread  
{//��EventLoop::runAt�Ⱥ�������  
    Timer* timer = new Timer(cb, when, interval);  
    loop_->runInLoop(bind(&TimerQueue::addTimerInLoop,this,timer));//ͨ��EventLoop::runInLoop()->TimerQueue::queueInLoop()  
    //runInLoop���������Ǳ�IO�߳���Ҫ��Ӷ�ʱ����ֱ����addTimerInLoop��ӣ����������߳���IO�߳���Ӷ�ʱ������Ҫ���ͨ��queueInLoop���  
    return TimerId(timer,timer->sequence());  
}  
void TimerQueue::addTimerInLoop(Timer* timer){//IO�߳��Լ����Լ���Ӷ�ʱ��  
    loop_->assertInLoopThread();  
    bool earliestChanged=insert(timer);//����ǰ����Ķ�ʱ���ȶ����еĶ�ʱ�������򷵻���  
    if(earliestChanged){  
        resetTimerfd(timerfd_,timer->expiration());//timerfd�������ó�ʱʱ��  
    }  
}  
void TimerQueue::cancel(TimerId timerId){  
    loop_->runInLoop(bind(&TimerQueue::cancelInLoop,this,timerId));  
}  
void TimerQueue::cancelInLoop(TimerId timerId){  
    loop_->assertInLoopThread();  
    assert(timers_.size()==activeTimers_.size());  
    ActiveTimer timer(timerId.timer_,timerId.sequence_);  
    ActiveTimerSet::iterator it=activeTimers_.find(timer);  
    if(it!=activeTimers_.end()){  
        size_t n=timers_.erase(Entry(it->first->expiration(),it->first));  
        assert(n==1);  
        (void)n;  
        delete it->first;  
        activeTimers_.erase(it);  
    }  
    else if(callingExpiredTimers_){  
        cancelingTimers_.insert(timer);  
    }  
    assert(timers_.size()==activeTimers_.size());  
}  
void TimerQueue::handleRead(){//timerfd�Ļص�����  
    loop_->assertInLoopThread();  
    Timestamp now(Timestamp::now());  
    readTimerfd(timerfd_, now);  
    std::vector<Entry> expired = getExpired(now);//TimerQueue::timerfd�ɶ������������ж�ʱ����ʱ������Ҫ�ҳ���Щ��ʱ�Ķ�ʱ��  
    callingExpiredTimers_=true;  
    cancelingTimers_.clear();  
    for (std::vector<Entry>::iterator it = expired.begin();it!= expired.end(); ++it)//  
    {  
        it->second->run();//ִ�ж�ʱ��Timer�ĳ�ʱ�ص�  
    }  
    callingExpiredTimers_=false;  
    reset(expired, now);//�鿴�Ѿ�ִ����ĳ��ж�ʱ���Ƿ���Ҫ�ٴζ�ʱ  
}  
std::vector<TimerQueue::Entry> TimerQueue::getExpired(Timestamp now)//��ȡ�����еĳ�ʱ�Ķ�ʱ��(���ܶ��)  
{  
    assert(timers_.size()==activeTimers_.size());  
    std::vector<Entry> expired;  
    Entry sentry = std::make_pair(now, reinterpret_cast<Timer*>(UINTPTR_MAX));  
    TimerList::iterator it = timers_.lower_bound(sentry);//���رȲ���С���½磬�����ص�һ����ǰδ��ʱ�Ķ�ʱ��(����û�������Ķ�ʱ��)  
    //lower_bound(value_type& val)����key_comp���ص�һ����С��val�ĵ�����  
    assert(it == timers_.end() || now < it->first);  
    std::copy(timers_.begin(), it, back_inserter(expired));  
    timers_.erase(timers_.begin(), it);  
    BOOST_FOREACH(Entry entry,expired){  
        ActiveTimer timer(entry.second,entry.second->sequence());  
        size_t n=activeTimers_.erase(timer);  
        assert(n==1);  
        (void)n;  
    }  
    return expired;//�����Ѿ���ʱ���ǲ��ֶ�ʱ��  
}  
  
void TimerQueue::reset(const std::vector<Entry>& expired, Timestamp now)//�Ѿ�ִ���곬ʱ�ص��Ķ�ʱ���Ƿ���Ҫ���ö�ʱ  
{  
    Timestamp nextExpire;  
    for (std::vector<Entry>::const_iterator it = expired.begin();it != expired.end(); ++it)  
    {  
        ActiveTimer timer(it->second,it->second->sequence());  
        if (it->second->repeat()&&  
                cancelingTimers_.find(timer)==cancelingTimers_.end()){//��Ҫ�ٴζ�ʱ  
            it->second->restart(now);  
            insert(it->second);  
        }  
        else{//����ɾ���ö�ʱ��  
            delete it->second;  
        }  
    }  
    if (!timers_.empty()){//Ϊ��ʱ��ʱ�����¶�ʱ����Ҫ��ȡ��ǰ��С�ĳ�ʱʱ���timerfd���Է����õ���Щ���ж�ʱ���к�����С�ĳ�ʱʱ��  
        nextExpire = timers_.begin()->second->expiration();  
    }  
    if (nextExpire.valid()){  
        resetTimerfd(timerfd_, nextExpire);//����timerfd�ĳ�ʱʱ��  
    }  
}  
bool TimerQueue::insert(Timer* timer)//��ʱ�����в��붨ʱ��  
{  
    loop_->assertInLoopThread();  
    assert(timers_.size()==activeTimers_.size());  
    bool earliestChanged = false;  
    Timestamp when = timer->expiration();  
    TimerList::iterator it = timers_.begin();  
    if (it == timers_.end() || when < it->first)  
    {  
        earliestChanged = true;//��ǰ����Ķ�ʱ���Ƕ�������С�Ķ�ʱ������ʱ��㺯����Ҫ����timerfd�ĳ�ʱʱ��  
    }  
    {  
        pair<TimerList::iterator,bool> result=  
            timers_.insert(Entry(when,timer));  
        assert(result.second);  
        (void)result;  
    }  
    {  
        pair<ActiveTimerSet::iterator,bool> result=  
            activeTimers_.insert(ActiveTimer(timer,timer->sequence()));  
        assert(result.second);  
        (void)result;  
    }  
    assert(timers_.size()==activeTimers_.size());  
    return earliestChanged;  
}  
  
/* 
*EventLoop��Աʵ�� 
*/  
class IngnoreSigPipe{  
    public:  
        IngnoreSigPipe(){  
            ::signal(SIGPIPE,SIG_IGN);  
        }  
};  
IngnoreSigPipe initObj;  
__thread EventLoop* t_loopInThisThread=0;//�߳�˽�����ݱ�ʾ�߳��Ƿ�ӵ��EventLoop  
const int kPollTimeMs=10000;//poll�ȴ�ʱ��  
static int createEventfd(){//����eventfd��eventfd���ڻ���  
    int evtfd=eventfd(0,EFD_NONBLOCK|EFD_CLOEXEC);  
    if(evtfd<0){  
        printf("Failed in eventfd\n");  
        abort();  
    }  
    return evtfd;  
}  
EventLoop::EventLoop()  
    :looping_(false),  
    quit_(false),  
    threadID_(syscall(SYS_gettid)),  
    poller_(new Poller(this)),  
    timerQueue_(new TimerQueue(this)),//EventLoop����һ����ʱ������  
    wakeupFd_(createEventfd()),  
    wakeupChannel_(new Channel(this,wakeupFd_)),//ͨ��Channel�۲�wakeupFd_  
    callingPendingFunctors_(false)  
{  
    if(!t_loopInThisThread){  
        t_loopInThisThread=this;//EventLoop����ʱ�߳�˽�����ݼ�¼  
    }  
    wakeupChannel_->setReadCallback(bind(&EventLoop::handleRead,this));//����eventfd�Ļص�  
    wakeupChannel_->enableReading();//eventfd�Ŀɶ��¼�,��Channel::update(this)��eventfd��ӵ�poll�¼�����  
}  
EventLoop::~EventLoop(){  
    assert(!looping_);  
    close(wakeupFd_);  
    t_loopInThisThread=NULL;//EventLoop���������ÿ�  
}  
void EventLoop::loop(){//EventLoop��ѭ������Ҫ�����Ǽ����¼����ϣ�ִ�о����¼��Ĵ�����  
    assert(!looping_);  
    assertInLoopThread();  
    looping_=true;  
    quit_=false;  
    while(!quit_){  
        activeChannels_.clear();  
        poller_->Poll(kPollTimeMs,&activeChannels_);//activeChannels�Ǿ����¼�  
        for(ChannelList::iterator it=activeChannels_.begin();it!=activeChannels_.end();it++){  
            (*it)->handleEvent();//��������¼��Ļص������������¼��ص�  
        }  
        doPendingFunctors();//�����û�����ص�  
    }  
    looping_=false;  
}  
void EventLoop::quit(){  
    quit_=true;//ֹͣ��ѭ����־����ѭ����������ֹͣ���ӳ�  
    if(!isInLoopThread()){  
        wakeup();//�����̻߳���EventLoop�߳�����ֹ��  
    }  
}  
void EventLoop::updateChannel(Channel* channel){//��Ҫ�����ļ���������ӵ�poll�ļ����¼�������  
    assert(channel->ownerLoop()==this);  
    assertInLoopThread();  
    poller_->updateChannel(channel);  
}  
void EventLoop::abortNotInLoopThread(){  
    printf("abort not in Loop Thread\n");  
    abort();//�Ǳ��̵߳���ǿ����ֹ  
}  
TimerId EventLoop::runAt(const Timestamp& time, const TimerCallback& cb)//����ʱ��ִ�лص�  
{  
    return timerQueue_->addTimer(cb, time, 0.0);  
}  
TimerId EventLoop::runAfter(double delay, const TimerCallback& cb)//���ʱ��ִ�лص�  
{  
    Timestamp time(addTime(Timestamp::now(), delay));  
    return runAt(time, cb);  
}  
TimerId EventLoop::runEvery(double interval, const TimerCallback& cb)//�����Իص�  
{  
    Timestamp time(addTime(Timestamp::now(), interval));//Timestamp::addTime  
    return timerQueue_->addTimer(cb, time, interval);  
}  
void EventLoop::cancel(TimerId timerId){  
    return timerQueue_->cancel(timerId);  
}  
void EventLoop::runInLoop(const Functor& cb){  
    if(isInLoopThread()){//��IO�̵߳�����ֱ��ִ��ִ���û��ص�  
       cb();  
    }  
    else{//�����̵߳���runInLoop�����û��ص�������ӣ���֤�̰߳�ȫone loop per thread  
        queueInLoop(cb);  
    }  
}  
void EventLoop::queueInLoop(const Functor& cb){  
    {  
        MutexLockGuard lock(mutex_);//�����������û��ص�����  
        pendingFunctors_.push_back(cb);  
    }  
    if(!isInLoopThread()||callingPendingFunctors_){  
        wakeup();//�����߳�����û��ص��������EventLoop��IO�߳����ڴ����û�����ص�ʱ������������IO�߳�  
    }  
}  
void EventLoop::handleRead(){//eventfd�ɶ��ص�  
    uint64_t one=1;  
    ssize_t n=read(wakeupFd_,&one,sizeof(one));  
    if(n!=sizeof(one)){  
        printf("EventLoop::handleRead() error\n");  
    }  
}  
void EventLoop::doPendingFunctors(){//ִ���û�����ص�  
    vector<Functor> functors;  
    callingPendingFunctors_=true;  
    {  
        MutexLockGuard lock(mutex_);  
        functors.swap(pendingFunctors_);//����swap������������ִ�лص���Ϊ����С�ٽ���  
    }  
    for(size_t i=0;i<functors.size();i++){  
        functors[i]();  
    }  
    callingPendingFunctors_=false;  
}  
void EventLoop::wakeup(){  
    uint64_t one=1;  
    ssize_t n=write(wakeupFd_,&one,sizeof(one));//ͨ��eventfd֪ͨ  
    if(n!=sizeof(one)){  
        printf("EventLoop::wakeup() write error\n");  
    }  
}  
void EventLoop::removeChannel(Channel* channel){  
    assert(channel->ownerLoop()==this);  
    assertInLoopThread();  
    poller_->removeChannel(channel);  
}  
  
/* 
*Poller��Աʵ�� 
*/  
Poller::Poller(EventLoop* loop):ownerLoop_(loop){}//Poller��ȷ������EventLoop  
Poller::~Poller(){}  
Timestamp Poller::Poll(int timeoutMs,ChannelList* activeChannels){  
    int numEvents=poll(&*pollfds_.begin(),pollfds_.size(),timeoutMs);//poll�����¼�����pollfds_  
    Timestamp now(Timestamp::now());  
    if(numEvents>0){  
        fillActiveChannels(numEvents,activeChannels);//���������¼���ӵ�activeChannels  
    }  
    else if(numEvents==0){  
    }  
    else{  
        printf("Poller::Poll error\n");  
    }  
    return now;  
}  
void Poller::fillActiveChannels(int numEvents,ChannelList* activeChannels) const{//�������¼�ͨ��activeChannels����  
    for(PollFdList::const_iterator pfd=pollfds_.begin();pfd!=pollfds_.end()&&numEvents>0;++pfd){  
        if(pfd->revents>0){  
            --numEvents;//��numEvents���¼�ȫ���ҵ��Ͳ���Ҫ�ٱ�������ʣ�µĲ���  
            ChannelMap::const_iterator ch=channels_.find(pfd->fd);  
            assert(ch!=channels_.end());  
            Channel* channel=ch->second;  
            assert(channel->fd()==pfd->fd);  
            channel->set_revents(pfd->revents);  
            activeChannels->push_back(channel);  
        }  
    }  
}  
void Poller::updateChannel(Channel* channel){  
    assertInLoopThread();  
    if(channel->index()<0){//��channel���ļ�������fdû����ӵ�poll�ļ����¼�������  
        assert(channels_.find(channel->fd())==channels_.end());  
        struct pollfd pfd;  
        pfd.fd=channel->fd();  
        pfd.events=static_cast<short>(channel->events());  
        pfd.revents=0;  
        pollfds_.push_back(pfd);  
        int idx=static_cast<int>(pollfds_.size())-1;  
        channel->set_index(idx);  
        channels_[pfd.fd]=channel;  
    }  
    else{//���Ѿ���ӵ������¼������У�������Ҫ�޸�  
        assert(channels_.find(channel->fd())!=channels_.end());  
        assert(channels_[channel->fd()]==channel);  
        int idx=channel->index();  
        assert(0<=idx&&idx<static_cast<int>(pollfds_.size()));  
        struct pollfd& pfd=pollfds_[idx];  
        assert(pfd.fd==channel->fd()||pfd.fd==-channel->fd()-1);//pfd.fd=-channel->fd()-1��Ϊ����poll������ЩkNoneEvent����������-1����Ϊ:fd����Ϊ0����-channel->fd()���ܻ���0,��������һ�������ܵ�������  
        pfd.events=static_cast<short>(channel->events());//�޸�ע���¼�����  
        pfd.revents=0;  
        if(channel->isNoneEvent()){  
            pfd.fd=-channel->fd()-1;//channel::events_=kNoneEventʱpoll������Щ�����ܵ�������-channel->fd()-1,-1ԭ�������  
        }  
    }  
}  
void Poller::removeChannel(Channel* channel)  
{  
  assertInLoopThread();  
  assert(channels_.find(channel->fd()) != channels_.end());  
  assert(channels_[channel->fd()] == channel);  
  assert(channel->isNoneEvent());  
  int idx = channel->index();  
  assert(0 <= idx && idx < static_cast<int>(pollfds_.size()));  
  const struct pollfd& pfd = pollfds_[idx]; (void)pfd;  
  assert(pfd.fd == -channel->fd()-1 && pfd.events == channel->events());  
  size_t n = channels_.erase(channel->fd());  
  assert(n == 1); (void)n;  
  if (implicit_cast<size_t>(idx) == pollfds_.size()-1) {  
    pollfds_.pop_back();  
  } else {  
    int channelAtEnd = pollfds_.back().fd;  
    iter_swap(pollfds_.begin()+idx, pollfds_.end()-1);  
    if (channelAtEnd < 0) {  
      channelAtEnd = -channelAtEnd-1;  
    }  
    channels_[channelAtEnd]->set_index(idx);  
    pollfds_.pop_back();  
  }  
}  
/* 
*Channel��Աʵ�� 
*/  
const int Channel::kNoneEvent=0;//���¼�  
const int Channel::kReadEvent=POLLIN|POLLPRI;//�ɶ��¼�  
const int Channel::kWriteEvent=POLLOUT;//��д�¼�  
Channel::Channel(EventLoop* loop,int fdArg)  
    :loop_(loop),fd_(fdArg),events_(0),revents_(0),  
    index_(-1),eventHandling_(false)  
    {}  
void Channel::update(){//��ӻ��޸��ļ����������¼�����  
    loop_->updateChannel(this);  
}  
Channel::~Channel(){  
    assert(!eventHandling_);  
}  
void Channel::handleEvent(){//��������¼��Ĵ�����  
    eventHandling_=true;  
    if(revents_&POLLNVAL){  
        printf("Channel::handleEvent() POLLNVAL\n");  
    }  
    if((revents_&POLLHUP)&&!(revents_&POLLIN)){//����ص�  
        printf("Channel::handle_event() POLLUP\n");  
        if(closeCallback_)  
            closeCallback_();  
    }  
    if(revents_&(POLLERR|POLLNVAL)){//�ɶ��ص�  
        if(errorCallback_)  
            errorCallback_();  
    }  
    if(revents_&(POLLIN|POLLPRI|POLLRDHUP)){  
        if(readCallback_) readCallback_();  
    }  
    if(revents_&POLLOUT){//��д�ص�  
        if(writeCallback_)  
            writeCallback_();  
    }  
    eventHandling_=false;  
}  
  
/* 
*����һ���߳�ִ��һ��EventLoop�������one loop per thread 
*/  
class EventLoopThread:noncopyable{  
    public:  
        EventLoopThread()  
            :loop_(NULL),exiting_(false),  
            thread_(bind(&EventLoopThread::threadFunc,this)),  
            mutex_(),cond_(mutex_){}  
        ~EventLoopThread(){  
            exiting_=true;  
            loop_->quit();  
            thread_.join();  
        }  
        EventLoop* startLoop(){  
            //assert(!thread_.started());  
            thread_.start();  
            {  
                MutexLockGuard lock(mutex_);  
                while(loop_==NULL){  
                    cond_.wait();  
                }  
            }  
            return loop_;  
        }  
    private:  
        void threadFunc(){  
            EventLoop loop;  
            {  
                MutexLockGuard lock(mutex_);  
                loop_=&loop;  
                cond_.notify();  
            }  
            loop.loop();  
        }  
        EventLoop* loop_;  
        bool exiting_;  
        Thread thread_;  
        Mutex mutex_;  
        Condition cond_;  
};  
/* 
 * EventLoopThreadPool 
 */  
class EventLoopThreadPool:noncopyable{  
    public:  
        EventLoopThreadPool(EventLoop* baseLoop)  
            :baseLoop_(baseLoop),  
            started_(false),numThreads_(0),next_(0){}  
        ~EventLoopThreadPool(){}  
        void setThreadNum(int numThreads){numThreads_=numThreads;}  
        void start(){  
            assert(!started_);  
            baseLoop_->assertInLoopThread();  
            started_=true;  
            for(int i=0;i<numThreads_;i++){  
                EventLoopThread* t=new EventLoopThread;  
                threads_.push_back(t);  
                loops_.push_back(t->startLoop());  
            }  
        }  
        EventLoop* getNextLoop(){  
            baseLoop_->assertInLoopThread();  
            EventLoop* loop=baseLoop_;  
            if(!loops_.empty()){  
                loop=loops_[next_];  
                ++next_;  
                if(static_cast<size_t>(next_)>=loops_.size())  
                    next_=0;  
            }  
            return loop;  
        }  
    private:  
        EventLoop* baseLoop_;  
        bool started_;  
        int numThreads_;  
        int next_;  
        ptr_vector<EventLoopThread> threads_;  
        vector<EventLoop*> loops_;  
};  
/* 
 *���õ�socketѡ�� 
 */  
namespace sockets{  
  
inline uint64_t hostToNetwork64(uint64_t host64)  
{//�����ֽ���תΪ�����ֽ���  
     return htobe64(host64);  
}  
inline uint32_t hostToNetwork32(uint32_t host32)  
{  
    return htonl(host32);  
}  
inline uint16_t hostToNetwork16(uint16_t host16)  
{  
    return htons(host16);  
}  
inline uint64_t networkToHost64(uint64_t net64)  
{//�����ֽ���תΪ�����ֽ���  
    return be64toh(net64);  
}  
  
inline uint32_t networkToHost32(uint32_t net32)  
{  
    return ntohl(net32);  
}  
inline uint16_t networkToHost16(uint16_t net16)  
{  
    return ntohs(net16);  
}  
  
typedef struct sockaddr SA;  
const SA* sockaddr_cast(const struct sockaddr_in* addr){//ǿ��ת��  
    return static_cast<const SA*>(implicit_cast<const void*>(addr));  
}  
SA* sockaddr_cast(struct sockaddr_in* addr){  
    return static_cast<SA*>(implicit_cast<void*>(addr));  
}  
void setNonBlockAndCloseOnExec(int sockfd){//������������Ϊ��������O_CLOEXEC(close on exec)  
    int flags = ::fcntl(sockfd, F_GETFL, 0);  
    flags |= O_NONBLOCK;  
    int ret = ::fcntl(sockfd, F_SETFL, flags);  
    flags = ::fcntl(sockfd, F_GETFD, 0);  
    flags |= FD_CLOEXEC;  
    ret = ::fcntl(sockfd, F_SETFD, flags);  
}  
int createNonblockingOrDie()  
{//socket()������������socket������  
    #if VALGRIND  
    int sockfd = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);  
    if (sockfd < 0) {  
        printf("socket() error\n");  
    }  
    setNonBlockAndCloseOnExec(sockfd);  
    #else  
    int sockfd = ::socket(AF_INET,  
                        SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC,  
                        IPPROTO_TCP);  
    if (sockfd < 0){  
        printf("socke() error\n");  
    }  
    #endif  
    return sockfd;  
}  
int connect(int sockfd,const struct sockaddr_in& addr){  
    return ::connect(sockfd,sockaddr_cast(&addr),sizeof addr);  
}  
void bindOrDie(int sockfd, const struct sockaddr_in& addr)  
{//bind()  
   int ret = ::bind(sockfd, sockaddr_cast(&addr), sizeof addr);  
     if (ret < 0) {  
         printf("bind() error\n");  
    }  
}  
void listenOrDie(int sockfd){//listen()  
    int ret = ::listen(sockfd, SOMAXCONN);  
    if (ret < 0){  
          printf("listen() error\n");  
    }  
}  
int accept(int sockfd, struct sockaddr_in* addr)  
{//accept()  
    socklen_t addrlen = sizeof *addr;  
    #if VALGRIND  
    int connfd = ::accept(sockfd, sockaddr_cast(addr), &addrlen);  
    setNonBlockAndCloseOnExec(connfd);  
    #else  
    int connfd = ::accept4(sockfd, sockaddr_cast(addr),  
                         &addrlen, SOCK_NONBLOCK | SOCK_CLOEXEC);  
    #endif  
    if (connfd < 0){  
        int savedErrno = errno;  
        printf("accept error\n");  
        switch (savedErrno)  
        {  
            case EAGAIN:  
            case ECONNABORTED:  
            case EINTR:  
            case EPROTO: // ???  
            case EPERM:  
            case EMFILE: // per-process lmit of open file desctiptor ???  
                errno = savedErrno;  
                break;  
            case EBADF:  
            case EFAULT:  
            case EINVAL:  
            case ENFILE:  
            case ENOBUFS:  
            case ENOMEM:  
            case ENOTSOCK:  
            case EOPNOTSUPP:  
                printf("accept() fatal erro\n");  
                break;  
            default:  
                printf("accpet() unknown error\n");  
                break;  
        }  
    }  
    return connfd;  
}  
void close(int sockfd){//close()  
    if (::close(sockfd) < 0){  
        printf("sockets::close\n");  
    }  
}  
void shutdownWrite(int sockfd){  
    if(::shutdown(sockfd,SHUT_WR)<0)  
        printf("sockets::shutdownWrite() error\n");  
}  
void toHostPort(char* buf, size_t size,const struct sockaddr_in& addr)  
{//��IPv4��ַתΪIP�Ͷ˿�  
    char host[INET_ADDRSTRLEN] = "INVALID";  
    ::inet_ntop(AF_INET, &addr.sin_addr, host, sizeof host);  
    uint16_t port =networkToHost16(addr.sin_port);  
    snprintf(buf, size, "%s:%u", host, port);  
}  
void fromHostPort(const char* ip, uint16_t port,struct sockaddr_in* addr)  
{//������IP�Ͷ˿�תΪIPv4��ַ  
    addr->sin_family = AF_INET;  
    addr->sin_port = hostToNetwork16(port);  
    if (::inet_pton(AF_INET, ip, &addr->sin_addr) <= 0)  
    {  
        printf("sockets::fromHostPort\n");  
    }  
}  
sockaddr_in getLocalAddr(int sockfd)  
{  
  struct sockaddr_in localaddr;  
  bzero(&localaddr, sizeof localaddr);  
  socklen_t addrlen = sizeof(localaddr);  
  if (::getsockname(sockfd, sockaddr_cast(&localaddr), &addrlen) < 0)  
  {  
      printf("getsockname() error\n");  
  }  
  return localaddr;  
}  
struct sockaddr_in getPeerAddr(int sockfd){  
    struct sockaddr_in peeraddr;  
    bzero(&peeraddr,sizeof peeraddr);  
    socklen_t addrlen=sizeof peeraddr;  
    if(::getpeername(sockfd,sockaddr_cast(&peeraddr),&addrlen)<0)  
        printf("sockets::getPeerAddr() error\n");  
    return peeraddr;  
}  
int getSocketError(int sockfd){  
    int optval;  
    socklen_t optlen=sizeof optval;  
    if(getsockopt(sockfd,SOL_SOCKET,SO_ERROR,&optval,&optlen)<0){  
        return errno;  
    }  
    else{  
        return optval;  
    }  
}  
bool isSelfConnect(int sockfd){//�������ж�  
    struct sockaddr_in localaddr=getLocalAddr(sockfd);  
    struct sockaddr_in peeraddr=getPeerAddr(sockfd);  
    return localaddr.sin_port==peeraddr.sin_port&&  
        localaddr.sin_addr.s_addr==peeraddr.sin_addr.s_addr;  
}  
}//end-namespace  
/* 
 * Socket 
 */  
class InetAddress;  
class Socket:noncopyable{//����һ��socket������fd����sockaddr������fd����  
    public:  
        explicit Socket(uint16_t sockfd):sockfd_(sockfd){}  
        ~Socket();  
        int fd() const{return sockfd_;}  
        void bindAddress(const InetAddress& addr);  
        void listen();  
        int accept(InetAddress* peeraddr);  
        void setReuseAddr(bool on);  
        void shutdownWrite(){  
            sockets::shutdownWrite(sockfd_);  
        }  
        void setTcpNoDelay(bool on){  
            int optval=on?1:0;  
            ::setsockopt(sockfd_,IPPROTO_TCP,TCP_NODELAY,&optval,sizeof optval);  
        }  
    private:  
        const int sockfd_;  
};  
/* 
 * sockaddr_in 
 */  
class InetAddress{//sockaddr��ַ�ķ�װ  
    public:  
        explicit InetAddress(uint16_t port);  
        InetAddress(const string& ip,uint16_t port);  
        InetAddress(const struct sockaddr_in& addr):addr_(addr){}  
        string toHostPort() const;  
        const struct sockaddr_in& getSockAddrInet() const{return addr_;}  
        void setSockAddrInet(const struct sockaddr_in& addr){addr_=addr;}  
    private:  
        struct sockaddr_in addr_;  
};  
BOOST_STATIC_ASSERT(sizeof(InetAddress)==sizeof(struct sockaddr_in));//����ʱ����  
class Acceptor:noncopyable{//����TCP���Ӳ�ִ����Ӧ�Ļص�  
    public://Acceptor��Ӧ����һ������˵ļ���socket������listenfd  
        typedef function<void(int sockfd,const InetAddress&)> NewConnectionCallback;  
        Acceptor(EventLoop* loop,const InetAddress& listenAddr);  
        void setNewConnectionCallback(const NewConnectionCallback& cb)  
        { newConnectionCallback_=cb;}  
        bool listening() const{return listening_;}  
        void listen();  
    private:  
        void handleRead();  
        EventLoop* loop_;  
        Socket acceptSocket_;//�����listenfd��ӦRAII��װ��socket������  
        Channel acceptChannel_;//����Channel�������˼����˿�listenfd,�������ΪChannel����accpetSocket_���fd  
        NewConnectionCallback newConnectionCallback_;  
        bool listening_;  
  
};  
/* 
 *Socketʵ�� 
 */  
Socket::~Socket()  
{  
    sockets::close(sockfd_);  
}  
void Socket::bindAddress(const InetAddress& addr)  
{  
    sockets::bindOrDie(sockfd_, addr.getSockAddrInet());  
}  
void Socket::listen()  
{  
    sockets::listenOrDie(sockfd_);  
}  
int Socket::accept(InetAddress* peeraddr)  
{  
    struct sockaddr_in addr;  
    bzero(&addr, sizeof addr);  
    int connfd = sockets::accept(sockfd_, &addr);  
    if (connfd >= 0)  
    {  
        peeraddr->setSockAddrInet(addr);  
    }  
    return connfd;  
}  
void Socket::setReuseAddr(bool on)  
{  
    int optval = on ? 1 : 0;  
    ::setsockopt(sockfd_, SOL_SOCKET, SO_REUSEADDR,  
               &optval, sizeof optval);  
}  
/* 
 *InetAddressʵ�� 
 */  
static const in_addr_t kInaddrAny=INADDR_ANY;//����������ֽ���IP��ַΪ0  
InetAddress::InetAddress(uint16_t port)  
{  
    bzero(&addr_, sizeof addr_);  
    addr_.sin_family = AF_INET;  
    addr_.sin_addr.s_addr = sockets::hostToNetwork32(kInaddrAny);  
    addr_.sin_port = sockets::hostToNetwork16(port);  
}  
InetAddress::InetAddress(const std::string& ip, uint16_t port)  
{  
    bzero(&addr_, sizeof addr_);  
    sockets::fromHostPort(ip.c_str(), port, &addr_);  
}  
string InetAddress::toHostPort() const  
{  
    char buf[32];  
    sockets::toHostPort(buf, sizeof buf, addr_);  
    return buf;  
}  
/* 
 *Acceptorʵ�� 
 */  
Acceptor::Acceptor(EventLoop* loop, const InetAddress& listenAddr)  
  : loop_(loop),  
    acceptSocket_(sockets::createNonblockingOrDie()),  
    acceptChannel_(loop, acceptSocket_.fd()),  
    listening_(false)  
{  
    acceptSocket_.setReuseAddr(true);  
    acceptSocket_.bindAddress(listenAddr);  
    acceptChannel_.setReadCallback(  
      boost::bind(&Acceptor::handleRead, this));  
}  
void Acceptor::listen()  
{  
    loop_->assertInLoopThread();  
    listening_ = true;  
    acceptSocket_.listen();  
    acceptChannel_.enableReading();  
}  
void Acceptor::handleRead()  
{  
    loop_->assertInLoopThread();  
    InetAddress peerAddr(0);  
    int connfd = acceptSocket_.accept(&peerAddr);  
    if (connfd >= 0) {  
        if (newConnectionCallback_) {  
            newConnectionCallback_(connfd, peerAddr);  
        } else {  
            sockets::close(connfd);  
        }  
    }  
}  
/* 
 *Buffer�������ݽ����뷢�� 
 */  
class Buffer{//copyable  
    public:  
        static const size_t kCheapPrepend=8;  
        static const size_t kInitialSize=1024;  
        Buffer():buffer_(kCheapPrepend+kInitialSize),  
            readerIndex_(kCheapPrepend),writerInex_(kCheapPrepend)  
        {  
            assert(readableBytes()==0);  
            assert(writeableBytes()==kInitialSize);  
            assert(prependableBytes()==kCheapPrepend);  
        }  
        void swap(Buffer& rhs){  
            buffer_.swap(rhs.buffer_);  
            std::swap(readerIndex_,rhs.readerIndex_);  
            std::swap(writerInex_,rhs.writerInex_);  
        }  
        size_t readableBytes() const{  
            return writerInex_-readerIndex_;  
        }//����Buffer�ж�������  
        size_t writeableBytes() const{  
            return buffer_.size()-writerInex_;  
        }//���ػ��ж���ʣ��ռ�  
        size_t prependableBytes() const{  
            return readerIndex_;  
        }//���ؿɶ�λ��  
        const char* peek() const{  
            return begin()+readerIndex_;  
        }//��һ���ɶ����ֽڴ�  
        void retrieve(size_t len){  
            assert(len<=readableBytes());  
            readerIndex_+=len;  
        }//һ����û�ж��꣬readerindex_�ƶ�  
        void retrieveUntil(const char* end){  
            assert(peek()<=end);  
            assert(end<=beginWrite());//beginwrite()���ص�һ����д��λ��  
            retrieve(end-peek());  
        }//�����ж���Buffer�пɶ��ֽ�  
        void retrieveAll(){  
            readerIndex_=kCheapPrepend;  
            writerInex_=kCheapPrepend;  
        }//����Buffer  
        std::string retrieveAsString(){  
            string str(peek(),readableBytes());  
            retrieveAll();  
            return str;  
        }//��string����Buffer�����ݣ�������Buffer  
        void append(const string& str){  
            append(str.data(),str.length());  
        }  
        void append(const char* data,size_t len){  
            ensureWriteableBytes(len);//�ռ䲻������makespace���ݻ����ڲ���Ų  
            std::copy(data,data+len,beginWrite());//copy(Input first,Input last,Output)  
            hasWritten(len);//����writerinex_  
        }  
        void append(const void* data,size_t len){  
            append(static_cast<const char*>(data),len);  
        }  
        void ensureWriteableBytes(size_t len){  
            if(writeableBytes()<len){  
                makeSpace(len);  
            }//��ʣ��ռ䲻���������·���ռ�  
            assert(writeableBytes()>=len);  
        }  
        char* beginWrite(){  
            return begin()+writerInex_;  
        }//����д��λ��  
        const char* beginWrite() const{  
            return begin()+writerInex_;  
        }  
        void hasWritten(size_t len){  
            writerInex_+=len;  
        }//����writerinex_  
        void prepend(const void* data,size_t len){  
            assert(len<=prependableBytes());  
            readerIndex_-=len;  
            const char* d=static_cast<const char*>(data);  
            std::copy(d,d+len,begin()+readerIndex_);  
        }//ǰ���������  
        void shrink(size_t reserve){  
            vector<char> buf(kCheapPrepend+readableBytes()+reserve);  
            std::copy(peek(),peek()+readableBytes(),buf.begin()+kCheapPrepend);  
            buf.swap(buffer_);  
        }//����Buffer��С  
        ssize_t readFd(int fd,int* savedErrno){  
            char extrabuf[65536];//ջ�ռ�,vector�ڶѿռ�  
            struct iovec vec[2];  
            const size_t writeable=writeableBytes();  
            vec[0].iov_base=begin()+writerInex_;  
            vec[0].iov_len=writeable;  
            vec[1].iov_base=extrabuf;  
            vec[1].iov_len=sizeof extrabuf;  
            const ssize_t n=readv(fd,vec,2);//readv���ж�  
            if(n<0){  
                *savedErrno=errno;  
            }  
            else if(implicit_cast<size_t>(n)<=writeable){  
                writerInex_+=n;  
            }//Buffer����ʣ��  
            else{  
                writerInex_=buffer_.size();  
                append(extrabuf,n-writeable);  
            }//Buffer������ջ�ռ�����append��BufferʹBuffer�������  
            return n;  
        }  
    private:  
        char* begin(){//.>*>&���ַ�  
            return &*buffer_.begin();  
        }  
        const char* begin() const{  
            return &*buffer_.begin();  
        }  
        void makeSpace(size_t len){//ensurewriteablebytes()->makespace()  
        //��ʣ��ռ�writeable()<lenʱ������  
            if(writeableBytes()+prependableBytes()<len+kCheapPrepend){  
                buffer_.resize(writerInex_+len);  
            }//(Buffer.size()-writerinex_ʣ��ռ�)+(readerindex_��һ���ɶ�λ��)<  
            //len+ǰ���С����ʱ����������Ų������д�ˣ���Ҫ׷��Buffer�Ĵ�С  
            else{//����ͨ����Ų����len��С��д��  
                assert(kCheapPrepend<readerIndex_);  
                size_t readable=readableBytes();  
                std::copy(begin()+readerIndex_,begin()+writerInex_,begin()+kCheapPrepend);//Buffer������������ǰ��Ų  
                readerIndex_=kCheapPrepend;//readerindex_�ص���ʼλ��  
                writerInex_=readerIndex_+readable;  
                assert(readable==readableBytes());  
            }  
        }  
    private:  
        vector<char> buffer_;  
        size_t readerIndex_;  
        size_t writerInex_;  
};  
  
class TcpConnection;//��ʾһ��TCP����  
typedef shared_ptr<TcpConnection> TcpConnectionPtr;//  
/* 
 *TcpConnection 
 */  
class TcpConnection:noncopyable,public enable_shared_from_this<TcpConnection>{  
    public:  
        TcpConnection(EventLoop* loop,const string& name,int sockfd,  
                const InetAddress& localAddr,const InetAddress& peerAddr);  
        ~TcpConnection();  
        EventLoop* getLoop() const{return loop_;}  
        const string& name() const{return name_;}  
        const InetAddress& localAddr(){return localAddr_;}  
        const InetAddress& peerAddress(){return peerAddr_;}  
        bool connected() const{return state_==kConnected;}  
        void send(const string& message);//������Ϣ,Ϊ���̰߳�ȫ������Tcpconnection::sendInLoop()  
        void shutdown();//�ر�TCP����,Ϊ�˱�֤�̰߳�ȫ������Tcpconnection:shutdownInloop()  
        void setTcpNoDelay(bool on);//�ر�Nagle�㷨  
        void setConnectionCallback(const ConnectionCallback& cb){  
            connectionCallback_=cb;  
        }//set*Callbackϵ�к��������û�ͨ��Tcpserver::set*Callbackָ������TcpServer::newConnection()����Tcpconnection����ʱ���ݸ�Tcpconnection::set*Callback����  
        void setMessageCallback(const MessageCallback& cb){  
            messageCallback_=cb;  
        }  
        void setWriteCompleteCallback(const WriteCompleteCallback& cb){  
            writeCompleteCallback_=cb;  
        }  
        void setCloseCallback(const CloseCallback& cb){  
        //��TcpServer��TcpClient���ã���������е�TcpConnectionPtr  
            closeCallback_=cb;  
        }  
        void connectEstablished();//����Channel::enableReading()��Poller�¼���ע���¼���������TcpConnection::connectionCallback_()����û�ָ�������ӻص�  
        //Acceptor::handleRead()->TcpServer::newConnection()->TcpConnection::connectEstablished()  
        void connectDestroyed();//�������ٺ���,����Channel::diableAll()ʹPoller��sockfd_���ԣ�������Eventloop::removeChannel()�Ƴ�sockfd_��Ӧ��Channel  
        //TcpServer::removeConnection()->EventLoop::runInLoop()->TcpServer::removeConnectionInLoop()->EventLoop::queueInLoop()->TcpConnection::connectDestroyed()  
        //����TcpConenction����ǰ���õ����һ�����������ڸ����û������ѶϿ�  
        //��TcpConenction״̬��ΪkDisconnected,Channel::diableAll(),connectioncallback_(),EventLoop::removeChannel()  
    private:  
        enum StateE{kConnecting,kConnected,kDisconnecting,kDisconnected,};  
        //Tcpconnection���ĸ�״̬:�������ӣ������ӣ����ڶϿ����ѶϿ�  
        void setState(StateE s){state_=s;}  
        void handleRead();  
        //Tcpconnection::handle*ϵ�к�������Poller����sockfd_�Ͼ����¼�����Channel::handelEvent()���õľ����¼��ص�����  
        void handleWrite();  
        void handleClose();  
        void handleError();  
        void sendInLoop(const string& message);  
        void shutdownInLoop();  
        EventLoop* loop_;  
        string name_;  
        StateE state_;  
        scoped_ptr<Socket> socket_;//TcpConnection��Ӧ���Ǹ�TCP�ͻ����ӷ�װΪsocket_  
        scoped_ptr<Channel> channel_;//TcpConnection��Ӧ��TCP�ͻ�����connfd����Channel����  
        InetAddress localAddr_;//TCP���Ӷ�Ӧ�ķ���˵�ַ  
        InetAddress peerAddr_;//TCP���ӵĿͻ��˵�ַ  
        ConnectionCallback connectionCallback_;  
        //�û�ָ�������ӻص�����,TcpServer::setConenctionCallback()->Acceptor::handleRead()->Tcpserver::newConnection()->TcpConnection::setConnectionCallback()  
        //��TcpServer::setConenctionCallback()�����û�ע������ӻص�����ͨ��Acceptor::handleRead()->Tcpserver::newConnection()�����û����ӻص���������Tcpconnection  
        MessageCallback messageCallback_;//�û�ָ������Ϣ��������Ҳ�Ǿ���Tcpserver����Tcpconnection  
        WriteCompleteCallback writeCompleteCallback_;  
        CloseCallback closeCallback_;  
        Buffer inputBuffer_;  
        Buffer outputBuffer_;  
};  
/* 
 *Tcpserver 
 */  
class TcpServer:noncopyable{//�������е�TCP����  
    public:  
        TcpServer(EventLoop* loop,const InetAddress& listenAddr);//����ʱ���и������˿ڵĵ�ַ  
        ~TcpServer();  
        void setThreadNum(int numThreads);  
        void start();  
        void setConnectionCallback(const ConnectionCallback& cb){  
            connectionCallback_=cb;  
        }//TCP�ͻ����ӻص���TcpConnection�TcpConnection::connectEstablished()->TcpConnection::connectionCallback_()  
        void setMessageCallback(const MessageCallback& cb){  
            messageCallback_=cb;  
        }//�˻ص�������TcpConnection::setMessageCallback()��ΪTcpConenction����Ϣ�ص�  
        void setWriteCompleteCallback(const WriteCompleteCallback& cb){  
            writeCompleteCallback_=cb;  
        }  
    private:  
        void newConnection(int sockfd,const InetAddress& peerAddr);  
        void removeConnection(const TcpConnectionPtr& conn);  
        void removeConnectionInLoop(const TcpConnectionPtr& conn);  
        typedef map<string,TcpConnectionPtr> ConnectionMap;  
        EventLoop* loop_;  
        const string name_;  
        scoped_ptr<Acceptor> acceptor_;//�����˿ڽ�������  
        scoped_ptr<EventLoopThreadPool> threadPool_;//����EventLoopThreadPool����TCP����  
        ConnectionCallback connectionCallback_;//����TcpConnection::setConnectionCallback(connectioncallback_)  
        MessageCallback messageCallback_;//����TcpConnection::setMessageCallback(messagecallback_)  
        WriteCompleteCallback writeCompleteCallback_;  
        bool started_;  
        int nextConnId_;//���ڱ�ʶTcpConnection��name_+nextConnId_�͹�����һ��TcpConnection������  
        ConnectionMap connections_;//��TcpServer���������TCP�ͻ����Ӵ�ŵ�����  
};  
/* 
 *TcpConnectionʵ�� 
 */  
TcpConnection::TcpConnection(EventLoop* loop,  
                             const std::string& nameArg,  
                             int sockfd,  
                             const InetAddress& localAddr,  
                             const InetAddress& peerAddr)  
    : loop_(loop),  
    name_(nameArg),  
    state_(kConnecting),  
    socket_(new Socket(sockfd)),  
    channel_(new Channel(loop, sockfd)),  
    localAddr_(localAddr),  
    peerAddr_(peerAddr)  
{  
    channel_->setReadCallback(bind(&TcpConnection::handleRead, this));  
    channel_->setWriteCallback(bind(&TcpConnection::handleWrite,this));  
    channel_->setCloseCallback(bind(&TcpConnection::handleClose,this));  
    channel_->setErrorCallback(bind(&TcpConnection::handleError,this));  
}  
TcpConnection::~TcpConnection()  
{  
    printf("TcpConnection::%s,fd=%d\n",name_.c_str(),channel_->fd());  
}  
void TcpConnection::send(const string& message){  
    cout<<"TcpConnection::send() ##"<<message<<endl;  
    if(state_==kConnected){  
        if(loop_->isInLoopThread()){  
            sendInLoop(message);  
        }  
        else{  
            loop_->runInLoop(bind(&TcpConnection::sendInLoop,this,message));  
        }  
    }  
}  
void TcpConnection::sendInLoop(const string& message){  
    //��TcpConnection��socket�Ѿ�ע���˿�д�¼���outputBuffer_�Ѿ�����������ֱ�ӵ���Buffer::append  
    //��socket��Channelû��ע��ɶ������outputbuffer_û�����ݴ����������ֱ����write����  
    //��writeһ����û�з����꣬��ʣ��������Ҫappend��outputbuffer_  
    //��writeһ���Է����������Ҫִ��writecompletecallback_  
    loop_->assertInLoopThread();  
    ssize_t nwrote=0;  
    cout<<message<<endl;  
    if(!channel_->isWriting()&&outputBuffer_.readableBytes()==0){  
        nwrote=write(channel_->fd(),message.data(),message.size());  
        if(nwrote>=0){  
            if(implicit_cast<size_t>(nwrote)<message.size()){  
                printf("I am going to write more data\n");  
            }  
            else if(writeCompleteCallback_){  
                loop_->queueInLoop(bind(writeCompleteCallback_,shared_from_this()));  
            }  
        }  
        else{  
            nwrote=0;  
            if(errno!=EWOULDBLOCK){  
                printf("TcpConnection::sendInLoop() error\n");  
            }  
        }  
    }  
    assert(nwrote>=0);  
    if(implicit_cast<size_t>(nwrote)<message.size()){  
        outputBuffer_.append(message.data()+nwrote,message.size()-nwrote);  
        if(!channel_->isWriting()){  
           channel_->enableWriting();  
        }  
    }  
}  
void TcpConnection::shutdown(){  
    if(state_==kConnected){  
        setState(kDisconnecting);  
        loop_->runInLoop(bind(&TcpConnection::shutdownInLoop,this));  
    }  
}  
void TcpConnection::shutdownInLoop(){  
    loop_->assertInLoopThread();  
    if(!channel_->isWriting()){  
        socket_->shutdownWrite();  
    }  
}  
void TcpConnection::setTcpNoDelay(bool on){  
    socket_->setTcpNoDelay(on);  
}  
void TcpConnection::connectEstablished()  
{  
    loop_->assertInLoopThread();  
    assert(state_ == kConnecting);  
    setState(kConnected);  
    channel_->enableReading();  
    connectionCallback_(shared_from_this());//���ӽ����ص�����  
}  
void TcpConnection::handleRead()  
{  
    int savedErrno=0;  
    ssize_t n =inputBuffer_.readFd(channel_->fd(),&savedErrno);//readv()  
    if(n>0)  
        messageCallback_(shared_from_this(),&inputBuffer_);  
    else if(n==0)  
        handleClose();  
    else{  
        errno=savedErrno;  
        printf("TcpConnection::hanleRead() error\n");  
        handleError();  
    }  
}  
void TcpConnection::handleWrite(){  
    loop_->assertInLoopThread();  
    if(channel_->isWriting()){  
        ssize_t n=write(channel_->fd(),outputBuffer_.peek(),outputBuffer_.readableBytes());  
        if(n>0){//peek()���ص�һ���ɶ����ֽڣ�readablebytes()����Buffer�����ݵĴ�С  
            outputBuffer_.retrieve(n);//readerindex_+=n����Buffer�Ķ�λ��  
            if(outputBuffer_.readableBytes()==0){//���Buffer�ﻹ������δ���͵Ļ�������������shutdownwrite���ǵȴ����ݷ��������shutdown  
                channel_->disableWriting();//��ֹbusy loop  
                if(writeCompleteCallback_){  
                    loop_->queueInLoop(bind(writeCompleteCallback_,shared_from_this()));  
                }  
                if(state_==kDisconnecting)  
                    shutdownInLoop();  
            }  
            else  
                printf("I am going to write more data\n");  
        }  
        else  
            printf("TcpConnection::handleWrite()\n");  
    }  
    else  
        printf("Connection is down,no more writing\n");  
}  
void TcpConnection::handleClose(){  
    loop_->assertInLoopThread();  
    assert(state_==kConnected||state_==kDisconnecting);  
    channel_->disableAll();  
    closeCallback_(shared_from_this());  
}  
void TcpConnection::handleError(){  
    int err=sockets::getSocketError(channel_->fd());  
    printf("TcpConnection::handleError() %d %s\n",err,strerror(err));  
}  
void TcpConnection::connectDestroyed(){  
    loop_->assertInLoopThread();  
    printf("TcpConnection::handleClose() state=%s\n",state_);  
    assert(state_==kConnected||state_==kDisconnected);  
    setState(kDisconnected);  
    channel_->disableAll();  
    connectionCallback_(shared_from_this());  
    loop_->removeChannel(get_pointer(channel_));  
}  
/* 
 *TcpServerʵ�� 
 */  
TcpServer::TcpServer(EventLoop* loop, const InetAddressEventLoop::removeChannel()  
    name_(listenAddr.toHostPort()),  
    acceptor_(new Acceptor(loop, listenAddr)),  
    threadPool_(new EventLoopThreadPool(loop)),  
    started_(false),  
    nextConnId_(1))  
{  
    acceptor_->setNewConnectionCallback(bind(&TcpServer::newConnection, this, _1, _2));  
}  
TcpServer::~TcpServer()  
{  
}  
void TcpServer::setThreadNum(int numThreads){  
    assert(numThreads>=0);  
    threadPool_->setThreadNum(numThreads);  
}  
void TcpServer::start()  
{  
    if (!started_)  
    {  
        started_ = true;  
    }  
  
    if (!acceptor_->listening())  
    {  
        loop_->runInLoop(bind(&Acceptor::listen, get_pointer(acceptor_)));  
    }//ͨ��EventLoop��������˵�listenfd,shared_ptr.hpp�е�get_pointer���ڷ���shared_ptr������������ָ��  
}  
void TcpServer::newConnection(int sockfd, const InetAddress& peerAddr)  
{//����Acceptor����һ�����Ӻ�ͨ���˻ص�֪ͨʹ����  
    loop_->assertInLoopThread();  
    char buf[32];  
    snprintf(buf, sizeof buf, "#%d", nextConnId_);  
    ++nextConnId_;  
    string connName = name_ + buf;  
    InetAddress localAddr(sockets::getLocalAddr(sockfd));  
    EventLoop* ioLoop=threadPool_->getNextLoop();//ѡһ��EventLoop��TcpConnection  
    TcpConnectionPtr conn(  
      new TcpConnection(ioLoop, connName, sockfd, localAddr, peerAddr));  
    connections_[connName]=conn;  
    conn->setConnectionCallback(connectionCallback_);//���ݸ�TcpConnection  
    conn->setMessageCallback(messageCallback_);  
    conn->setWriteCompleteCallback(writeCompleteCallback_);  
    conn->setCloseCallback(bind(&TcpServer::removeConnection,this,_1));//���Ƴ�TcpConnectionPtr�Ĳ���ע�ᵽTcpConnection::setCloseCallback  
    ioLoop->runInLoop(bind(&TcpConnection::connectEstablished,conn));  
    //ͨ��EventLoop::runInLoop()->EventLoop::queueInLoop()->TcpConnection::connectEstablished()  
}  
void TcpServer::removeConnection(const TcpConnectionPtr& conn){  
    loop_->runInLoop(bind(&TcpServer::removeConnectionInLoop,this,conn));  
    //TcpServer::removeConnection()->EventLoop::runInLoop()->EventLoop::queueInLoop()->TcpServer::removeConnectionInLoop()  
}  
void TcpServer::removeConnectionInLoop(const TcpConnectionPtr& conn){  
    loop_->assertInLoopThread();  
    size_t n=connections_.erase(conn->name());  
    assert(n==1);  
    (void)n;  
    EventLoop* ioLoop=conn->getLoop();  
    ioLoop->queueInLoop(bind(&TcpConnection::connectDestroyed,conn));//��IO�߳������ֱ��EventLoop::queueInLoop()  
}  
/* 
 * �������� 
 */  
class Connector : boost::noncopyable  
{  
    public:  
        typedef function<void (int sockfd)> NewConnectionCallback;  
        Connector(EventLoop* loop, const InetAddress& serverAddr);  
        ~Connector();  
        void setNewConnectionCallback(const NewConnectionCallback& cb)  
        { newConnectionCallback_ = cb; }  
        void start();  // can be called in any thread  
        void restart();  // must be called in loop thread  
        void stop();  // can be called in any thread  
        const InetAddress& serverAddress() const { return serverAddr_; }  
    private:  
        enum States { kDisconnected, kConnecting, kConnected };  
        //δ���ӣ��������ӣ�������  
        static const int kMaxRetryDelayMs = 30*1000;  
        static const int kInitRetryDelayMs = 500;  
        void setState(States s) { state_ = s; }  
        void startInLoop();  
        void connect();  
        void connecting(int sockfd);  
        void handleWrite();  
        void handleError();  
        void retry(int sockfd);  
        int removeAndResetChannel();  
        void resetChannel();  
        EventLoop* loop_;  
        InetAddress serverAddr_;  
        bool connect_; // atomic  
        States state_;  // FIXME: use atomic variable  
        boost::scoped_ptr<Channel> channel_;  
        NewConnectionCallback newConnectionCallback_;  
        int retryDelayMs_;  
        TimerId timerId_;  
};  
/* 
 * Connectorʵ�� 
 */  
typedef boost::shared_ptr<Connector> ConnectorPtr;  
const int Connector::kMaxRetryDelayMs;  
Connector::Connector(EventLoop* loop, const InetAddress& serverAddr)  
    :loop_(loop),  
    serverAddr_(serverAddr),  
    connect_(false),  
    state_(kDisconnected),  
    retryDelayMs_(kInitRetryDelayMs)  
{  
}  
Connector::~Connector()  
{  
    loop_->cancel(timerId_);  
    assert(!channel_);  
}  
void Connector::start()  
{//�����������̵߳���  
    connect_ = true;  
    loop_->runInLoop(boost::bind(&Connector::startInLoop, this)); // FIXME: unsafe  
}  
void Connector::startInLoop()  
{  
    loop_->assertInLoopThread();  
    assert(state_ == kDisconnected);  
    if (connect_)  
    {  
        connect();//  
    }  
    else  
    {}  
}  
void Connector::connect()  
{  
    int sockfd = sockets::createNonblockingOrDie();  
    int ret = sockets::connect(sockfd, serverAddr_.getSockAddrInet());  
    int savedErrno = (ret == 0) ? 0 : errno;  
    switch (savedErrno)  
    {  
        case 0:  
        case EINPROGRESS:  
        case EINTR:  
        case EISCONN:  
            connecting(sockfd);  
            break;  
  
        case EAGAIN:  
        case EADDRINUSE:  
        case EADDRNOTAVAIL:  
        case ECONNREFUSED:  
        case ENETUNREACH:  
            retry(sockfd);  
            break;  
  
        case EACCES:  
        case EPERM:  
        case EAFNOSUPPORT:  
        case EALREADY:  
        case EBADF:  
        case EFAULT:  
        case ENOTSOCK:  
            sockets::close(sockfd);  
            break;  
  
        default:  
            sockets::close(sockfd);  
            // connectErrorCallback_();  
            break;  
  }  
}  
void Connector::restart()  
{  
    loop_->assertInLoopThread();  
    setState(kDisconnected);  
    retryDelayMs_ = kInitRetryDelayMs;  
    connect_ = true;  
    startInLoop();  
}  
void Connector::stop()  
{  
    connect_ = false;  
    loop_->cancel(timerId_);  
}  
void Connector::connecting(int sockfd)  
{//EINPROGRESS  
    setState(kConnecting);  
    assert(!channel_);  
    channel_.reset(new Channel(loop_, sockfd));  
    channel_->setWriteCallback(bind(&Connector::handleWrite, this)); // FIXME: unsafe  
    channel_->setErrorCallback(bind(&Connector::handleError, this)); // FIXME: unsafe  
    channel_->enableWriting();  
}  
int Connector::removeAndResetChannel()  
{  
    channel_->disableAll();  
    loop_->removeChannel(get_pointer(channel_));  
    int sockfd = channel_->fd();  
    loop_->queueInLoop(bind(&Connector::resetChannel, this)); // FIXME: unsafe  
    return sockfd;  
}  
void Connector::resetChannel()  
{  
    channel_.reset();  
}  
void Connector::handleWrite()  
{  
    if (state_ == kConnecting)  
    {  
        int sockfd = removeAndResetChannel();  
        int err = sockets::getSocketError(sockfd);  
        if (err)  
            retry(sockfd);  
        else if (sockets::isSelfConnect(sockfd))  
            retry(sockfd);  
        else  
        {  
            setState(kConnected);  
            if (connect_)  
                newConnectionCallback_(sockfd);  
            else  
                sockets::close(sockfd);  
        }  
    }  
    else  
    {  
        assert(state_ == kDisconnected);  
    }  
}  
  
void Connector::handleError()  
{  
    assert(state_ == kConnecting);  
  
    int sockfd = removeAndResetChannel();  
    int err = sockets::getSocketError(sockfd);  
    retry(sockfd);  
}  
  
void Connector::retry(int sockfd)  
{//EAGAIN  
    sockets::close(sockfd);  
    setState(kDisconnected);  
    if (connect_){  
        timerId_ = loop_->runAfter(retryDelayMs_/1000.0,  // FIXME: unsafe  
                               boost::bind(&Connector::startInLoop, this));  
        retryDelayMs_ = std::min(retryDelayMs_ * 2, kMaxRetryDelayMs);  
    }  
    else  
    {}  
}  
/* 
 * TcpClient 
 */  
typedef boost::shared_ptr<Connector> ConnectorPtr;  
class TcpClient : boost::noncopyable  
{  
    public:  
        TcpClient(EventLoop* loop,  
            const InetAddress& serverAddr,  
            const string& name);  
     ~TcpClient();  // force out-line dtor, for scoped_ptr members.  
        void connect();  
        void disconnect();  
        void stop();  
        TcpConnectionPtr connection() const  
        {  
            MutexLockGuard lock(mutex_);  
            return connection_;  
        }  
  
        EventLoop* getLoop() const { return loop_; }  
        bool retry() const;  
        void enableRetry() { retry_ = true; }  
        void setConnectionCallback(const ConnectionCallback& cb)  
        { connectionCallback_ = cb; }  
        void setMessageCallback(const MessageCallback& cb)  
        { messageCallback_ = cb; }  
        void setWriteCompleteCallback(const WriteCompleteCallback& cb)  
        { writeCompleteCallback_ = cb; }  
        #ifdef __GXX_EXPERIMENTAL_CXX0X__  
        void setConnectionCallback(ConnectionCallback&& cb)  
        { connectionCallback_ = cb; }  
        void setMessageCallback(MessageCallback&& cb)  
        { messageCallback_ = cb; }  
        void setWriteCompleteCallback(WriteCompleteCallback&& cb)  
        { writeCompleteCallback_ = cb; }  
        #endif  
    private:  
        void newConnection(int sockfd);  
        void removeConnection(const TcpConnectionPtr& conn);  
        EventLoop* loop_;  
        ConnectorPtr connector_; // avoid revealing Connector  
        const string name_;  
        ConnectionCallback connectionCallback_;  
        MessageCallback messageCallback_;  
        WriteCompleteCallback writeCompleteCallback_;  
        bool retry_;   // atmoic  
        bool connect_; // atomic  
        int nextConnId_;  
        mutable MutexLock mutex_;  
        TcpConnectionPtr connection_; // @BuardedBy mutex_  
};  
namespace detail  
{  
    void removeConnection(EventLoop* loop, const TcpConnectionPtr& conn)  
    {  
      loop->queueInLoop(boost::bind(&TcpConnection::connectDestroyed, conn));  
    }  
    void removeConnector(const ConnectorPtr& connector)  
    {  
      //connector->  
    }  
}  
TcpClient::TcpClient(EventLoop* loop,  
                     const InetAddress& serverAddr,  
                     const string& name)  
  : loop_(CHECK_NOTNULL(loop)),  
    connector_(new Connector(loop, serverAddr)),  
    name_(name),  
    connectionCallback_(defaultConnectionCallback),  
    messageCallback_(defaultMessageCallback),  
    retry_(false),  
    connect_(true),  
    nextConnId_(1)  
{  
    connector_->setNewConnectionCallback(  
      boost::bind(&TcpClient::newConnection, this, _1));  
}  
  
TcpClient::~TcpClient()  
{  
    TcpConnectionPtr conn;  
    {  
        MutexLockGuard lock(mutex_);  
        conn = connection_;  
    }  
    if (conn)  
    {  
        CloseCallback cb = boost::bind(&detail::removeConnection, loop_, _1);  
        loop_->runInLoop(  
            boost::bind(&TcpConnection::setCloseCallback, conn, cb));  
    }  
    else  
    {  
        connector_->stop();  
        loop_->runAfter(1, boost::bind(&detail::removeConnector, connector_));  
    }  
}  
void TcpClient::connect()  
{  
    connect_ = true;  
    connector_->start();  
}  
void TcpClient::disconnect()  
{  
    connect_ = false;  
    {  
        MutexLockGuard lock(mutex_);  
        if (connection_)  
        {  
            connection_->shutdown();  
        }  
    }  
}  
void TcpClient::stop()  
{  
    connect_ = false;  
    connector_->stop();  
}  
void TcpClient::newConnection(int sockfd)  
{  
    loop_->assertInLoopThread();  
    InetAddress peerAddr(sockets::getPeerAddr(sockfd));  
    char buf[32];  
    snprintf(buf, sizeof buf, ":%s#%d", peerAddr.toIpPort().c_str(), nextConnId_);  
    ++nextConnId_;  
    string connName = name_ + buf;  
    InetAddress localAddr(sockets::getLocalAddr(sockfd));  
    TcpConnectionPtr conn(new TcpConnection(loop_,  
                                          connName,  
                                          sockfd,  
                                          localAddr,  
                                          peerAddr));  
  
    conn->setConnectionCallback(connectionCallback_);  
    conn->setMessageCallback(messageCallback_);  
    conn->setWriteCompleteCallback(writeCompleteCallback_);  
    conn->setCloseCallback(  
      boost::bind(&TcpClient::removeConnection, this, _1)); // FIXME: unsafe  
    {  
        MutexLockGuard lock(mutex_);  
        connection_ = conn;  
    }  
    conn->connectEstablished();  
}  
void TcpClient::removeConnection(const TcpConnectionPtr& conn)  
{  
    loop_->assertInLoopThread();  
    assert(loop_ == conn->getLoop());  
    {  
        MutexLockGuard lock(mutex_);  
        assert(connection_ == conn);  
        connection_.reset();  
    }  
    loop_->queueInLoop(boost::bind(&TcpConnection::connectDestroyed, conn));  
    if (retry_ && connect_)  
    {  
        connector_->restart();  
    }  
}  
  
/* 
 *Epoll 
 */  
class Epoller:noncopyable{  
    public:  
        typedef vector<Channel*> ChannelList;  
        Epoller(EventLoop* loop)  
            :ownerLoop_(loop),  
            epollfd_(::epoll_create1(EPOLL_CLOEXEC)),  
            events_(kInitEventListSize)  
        {  
            if(epollfd_<0){  
                printf("Epoller::epoll_create1() error\n");  
                abort();  
            }  
        }  
        ~Epoller(){  
            ::close(epollfd_);  
        }  
        Timestamp poll(int timeoutMs,ChannelList* activeChannels){  
            int numEvents=::epoll_wait(epollfd_,&*events_.begin(),  
                    static_cast<int>(events_.size()),timeoutMs);  
            Timestamp now(Timestamp::now());  
            if(numEvents>0){  
                fillActiveChannels(numEvents,activeChannels);  
                if(implicit_cast<size_t>(numEvents)==events_.size()){  
                    events_.resize(events_.size()*2);  
                }  
                else if(numEvents==0){}  
                else{  
                    printf("Epoller::epoll_wait() error\n");  
                }  
            }  
            return now;  
        }  
        void updateChannel(Channel* channel){  
            assertInLoopThread();  
            const int index=channel->index();  
            if(index==-1||index==2){  
                int fd=channel->fd();  
                if(index==-1){  
                    assert(channels_.find(fd)==channels_.end());  
                    channels_[fd]=channel;  
                }  
                else{  
                    assert(channels_.find(fd)!=channels_.end());  
                    assert(channels_[fd]==channel);  
                }  
                channel->set_index(1);  
                update(EPOLL_CTL_ADD,channel);  
            }  
            else{  
                int fd=channel->fd();  
                (void)fd;  
                assert(channels_.find(fd)!=channels_.end());  
                assert(channels_[fd]==channel);  
                assert(index==1);  
                if(channel->isNoneEvent()){  
                    update(EPOLL_CTL_DEL,channel);  
                    channel->set_index(2);  
                }  
                else{  
                    update(EPOLL_CTL_MOD,channel);  
                }  
            }  
        }  
        void removeChannel(Channel* channel){  
            assertInLoopThread();  
            int fd=channel->fd();  
            assert(channels_.find(fd)!=channels_.end());  
            assert(channels_[fd]==channel);  
            assert(channel->isNoneEvent());  
            int index=channel->index();  
            assert(index==1||index==2);  
            size_t n=channels_.erase(fd);  
            (void)n;  
            assert(n==1);  
            if(index==1){  
                update(EPOLL_CTL_DEL,channel);  
            }  
            channel->set_index(-1);  
        }  
        void assertInLoopThread(){  
            ownerLoop_->assertInLoopThread();  
        }  
    private:  
        static const int kInitEventListSize=16;  
        void fillActiveChannels(int numEvents,ChannelList* activeChannels) const  
        {  
            assert(implicit_cast<size_t>(numEvents)<=events_.size());  
            for(int i=0;i<numEvents;i++){  
                Channel* channel=static_cast<Channel*>(events_[i].data.ptr);  
                int fd=channel->fd();  
                ChannelMap::const_iterator it=channels_.find(fd);  
                assert(it!=channels_.end());  
                assert(it->second==channel);  
                channel->set_revents(events_[i].events);  
                activeChannels->push_back(channel);  
            }  
        }  
        void update(int operation,Channel* channel){  
            struct epoll_event event;  
            bzero(&event,sizeof event);  
            event.events=channel->events();  
            event.data.ptr=channel;  
            int fd=channel->fd();  
            if(::epoll_ctl(epollfd_,operation,fd,&event)<0){  
                if(operation==EPOLL_CTL_DEL){  
                    printf("Epoller::update() EPOLL_CTL_DEL error\n");  
                }  
                else{  
                    printf("Epoller::update() EPOLL_CTL_ error\n");  
                }  
            }  
        }  
        typedef vector<struct epoll_event> EventList;  
        typedef map<int,Channel*> ChannelMap;  
  
        EventLoop* ownerLoop_;  
        int epollfd_;  
        EventList events_;  
        ChannelMap channels_;  
};  
