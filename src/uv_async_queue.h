#ifndef SRC_UV_ASYNC_QUEUE_H
#define SRC_UV_ASYNC_QUEUE_H

#include <uv.h>
#include <node_version.h>
#include <functional>
#include <queue>
#include <thread>
#include <mutex>

template <class T>
class UVQueue{
	public:
		UVQueue(std::function<void(T&)> cb, bool _keep_alive=false): callback(cb), keep_alive(_keep_alive) {
			uv_async_init(uv_default_loop(), &async, UVQueue::internal_callback);
			async.data = this;
			if (!keep_alive) unref();
		}
		
		void post(T value){
			mutex.lock();
			queue.push(value);
			mutex.unlock();
			uv_async_send(&async);
		}
		
		~UVQueue(){
			if (!keep_alive) ref();
			uv_close((uv_handle_t*)&async, NULL); //TODO: maybe we can't delete UVQueue until callback?
		}

		void ref(){
			#if NODE_VERSION_AT_LEAST(0, 7, 9)
			uv_ref((uv_handle_t*)&async);
			#else
			uv_ref(uv_default_loop());
			#endif
		}

		void unref(){
			#if NODE_VERSION_AT_LEAST(0, 7, 9)
			uv_unref((uv_handle_t*)&async);
			#else
			uv_unref(uv_default_loop());
			#endif
		}
		
	private:
		std::function<void(T&)> callback;
		std::queue<T> queue;
		std::mutex mutex;
		uv_async_t async;
		bool keep_alive;
		
		static void internal_callback(uv_async_t *handle, int status){
			UVQueue* uvqueue = static_cast<UVQueue*>(handle->data);
			
			while(1){
				uvqueue->mutex.lock();
				if (uvqueue->queue.empty()){
					uvqueue->mutex.unlock();
					break;
				}
				T item = uvqueue->queue.front();
				uvqueue->queue.pop();
				uvqueue->mutex.unlock();
				uvqueue->callback(item);
			}
		}
};

#endif
