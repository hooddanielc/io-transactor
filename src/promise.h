#pragma once

#include <cassert>
#include <condition_variable>
#include <mutex>

#include "json.h"

namespace dj {

  /* A promise of future JSON. */
  class promise_t final {
    public:

    /* Convenience. */
    using ptr_t = std::shared_ptr<promise_t>;

    /* An authority which makes and keeps promises. */
    class keeper_t final {
      public:

      /* Do-little. */
      keeper_t() noexcept
          : next_id(100001) {}

      /* Fulfill a promise previously made.  This will unblock any thread
         currently waiting for the promise and prevent any thread from
         waiting for it again. */
      void keep_promise(int id, json_t &&json) {
        assert(this);
        assert(&json);
        auto ptr = try_pop_promise(id);
        if (ptr) {
          ptr->keep(std::move(json));
        }
      }

      /* Make a new promise, not yet kept. */
      std::shared_ptr<promise_t> make_promise() {
        assert(this);
        std::unique_lock<std::mutex> lock(mutex);
        auto id = next_id++;
        auto result = std::make_shared<promise_t>(id);
        promises[id] = result;
        return result;
      }

      private:

      /* Locate the promise in our set, remove it, and return it.  If we
         don't have a promise with this id, return a null pointer. */
      ptr_t try_pop_promise(int id) {
        assert(this);
        ptr_t result;
        std::unique_lock<std::mutex> lock(mutex);
        auto iter = promises.find(id);
        if (iter != promises.end()) {
          result = iter->second;
          promises.erase(iter);
        }
        return result;
      }

      /* Covers next_id and promises. */
      std::mutex mutex;

      /* The id of the next promise we will make.  This starts at a nominal
         value and increases monotonically. */
      int next_id;

      /* The promises we've made but not yet kept. */
      std::map<int, ptr_t> promises;

    };  // promise_t::keeper_t

    /* No copying or moving. */
    promise_t(const promise_t &) = delete;
    promise_t &operator=(const promise_t &) = delete;

    /* Caches the id.  We start out as an unkept promise. */
    explicit promise_t(int id) noexcept
        : id(id), kept(false) {}

    /* Do-little. */
    ~promise_t() {}

    /* The id of this promise, unique among all promises made by the same
       keeper. */
    int get_id() const noexcept {
      assert(this);
      return id;
    }

    /* Wait for the promise to be kept, then return the JSON we've been
       waiting for.  If the promise has already been kept, return
       immediately. */
    const json_t &sync() const {
      assert(this);
      std::unique_lock<std::mutex> lock(mutex);
      while (!kept) {
        kept_set.wait(lock);
      }
      return json;
    }

    /* True iff. this promise has been kept already. */
    bool is_kept() const {
      assert(this);
      std::unique_lock<std::mutex> lock(mutex);
      return kept;
    }

    private:

    /* Take over the json object as our value and unblock any threads which
       have been waiting for it. */
    void keep(json_t &&json) {
      assert(this);
      assert(&json);
      std::unique_lock<std::mutex> lock(mutex);
      if (!kept) {
        kept = true;
        this->json = std::move(json);
        kept_set.notify_all();
      }
    }

    /* Set at construction and never changed. */
    const int id;

    /* Covers kept and json. */
    mutable std::mutex mutex;

    /* Starts out false and becomes true when kept() is called. */
    bool kept;

    /* Set once by kept() then never changed. */
    json_t json;

    /* Unblocks all threads waiting on sync(). */
    mutable std::condition_variable kept_set;

  };  // promise_t;

  /* Convenience. */
  using future_t = promise_t::ptr_t;

}  // dj
