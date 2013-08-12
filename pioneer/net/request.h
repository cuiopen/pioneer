/**
 * request.h
 *
 *  Created on: Sep 12, 2011
 *      Author: Vincent Zhang, ivincent.zhang@gmail.com
 */

/*    Copyright 2011 ~ 2013 Vincent Zhang, ivincent.zhang@gmail.com
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */

#ifndef PIONEER_NET_REQUEST_H_
#define PIONEER_NET_REQUEST_H_

#include <string>
#include <deque>

#include <glog/logging.h>
#include <boost/uuid/uuid.hpp>
#include <atlas/rpc.h>

#include <pioneer/system/context.h>
#include <pioneer/net/ip.h>
#include <pioneer/net/rpc_clients.h>

namespace pioneer {
  namespace net {

    using std::string;
    using boost::uuids::uuid;

    enum class error_category {
      no_error = 0,
      net_error,
      rpc_error,
      system_error,
      std_error,
      lib_3rd_error,
      string_error,
      cstring_error,
      unknown_error
    };

    class session;
    typedef std::shared_ptr<session> session_ptr;

    /**
     * @see session
     * */
    class request {
    public:

      request(const uuid& session_id, const session_ptr& s, const char* msg, size_t msg_size, const string& source_ip_port) :
          _message(msg, msg_size), _session(s), _source_ip_port(source_ip_port)
      {}

    public:

      session_ptr session() const { return _session.lock(); }

      void execute() noexcept {
        rpc::p2p_client response_client(static_cast<rpc::client_type>(_message.header()->client_id), _source_ip_port);
        atlas::rpc::dispatcher_manager::ref().execute(response_client, _message, _source_ip_port);
      }

    private:

      atlas::rpc::message _message;
      std::weak_ptr<pioneer::net::session> _session;

      std::string _source_ip_port;
    };

    typedef std::shared_ptr<request> request_ptr;

    class session : public std::enable_shared_from_this<session> {
    public:

      session(const uuid& id) : _id(id) { }

      ~session() { }

      const uuid& id() const { return _id; }

    public:

      void build_request(const char* message, size_t size, const std::string& source_ip_port) {
        _request.reset(new net::request(_id, shared_from_this(), message, size, source_ip_port));
      }

      const request_ptr& request() const { return _request; }

    private:

      /*
       * Session id must be an uuid, since the session is cluster-global and can be generated by
       * different host
       * */
      uuid _id;
      request_ptr _request;
    };

    bool operator<(const session& lhs, const session& rhs) {
      return lhs.id() < rhs.id();
    }

    bool operator==(const session& lhs, const session& rhs) {
      return lhs.id() == rhs.id();
    }

    class session_manager : public atlas::singleton<session_manager> {
    private:

      friend class atlas::singleton<session_manager>;
      session_manager(session_manager&)= delete;
      session_manager& operator=(const session_manager&)= delete;

    public:

      // TODO : make it private, and allow singleton to access it only
      session_manager() = default;

    public:

      const request_ptr& build_request(const std::string& source_ip_port, const char* data, size_t len) {
        const uuid& session_id = atlas::rpc::message::get_session_id(data, len);

        // DLOG(INFO) << "session : " << session_id;

        session_ptr s;
        bool found = false;

        // critical area
        {
          std::lock_guard<std::mutex> guard(_mutex);
          auto it = _sessions.find(session_id);
          found = (it != _sessions.end());

          if (found) {
            s = it->second;
          }
          else {
            // there is a session token, but we can not find a session in this node,
            // this means this is a request from an inner-cluster-client, we should
            // create one session with the pass-in session id
            s.reset(new session(session_id));
            _sessions.insert(std::make_pair(s->id(), s));
          }
        }

        s->build_request(data, len, source_ip_port);

        return s->request();
      }

      session_ptr get(const uuid& session_id) const {
        std::lock_guard<std::mutex> guard(_mutex);

        session_ptr s;
        auto it = _sessions.find(session_id);
        if (it != _sessions.end()) {
          s = it->second;
        }

        return s;
      }

      void remove(const uuid& session_id) {
        std::lock_guard<std::mutex> guard(_mutex);

        auto it = _sessions.find(session_id);
        if(it != _sessions.end()) {
          // if(it->second->request()->response()->is_cluster_canremove())
          _sessions.erase(session_id);
        }
      }

      size_t size() const {
        std::lock_guard<std::mutex> guard(_mutex);
        return _sessions.size();
      }

      void clear() {
        std::lock_guard<std::mutex> guard(_mutex);
        _sessions.clear();
      }

    private:

      mutable std::mutex _mutex;
      std::map<uuid, session_ptr> _sessions;
    };

  } // db
} // pioneer

#endif /* NET_REQUEST_H_ */
