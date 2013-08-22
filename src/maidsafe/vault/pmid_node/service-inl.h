/* Copyright 2012 MaidSafe.net limited

This MaidSafe Software is licensed under the MaidSafe.net Commercial License, version 1.0 or later,
and The General Public License (GPL), version 3. By contributing code to this project You agree to
the terms laid out in the MaidSafe Contributor Agreement, version 1.0, found in the root directory
of this project at LICENSE, COPYING and CONTRIBUTOR respectively and also available at:

http://www.novinet.com/license

Unless required by applicable law or agreed to in writing, software distributed under the License is
distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or
implied. See the License for the specific language governing permissions and limitations under the
License.
*/

#ifndef MAIDSAFE_VAULT_PMID_NODE_SERVICE_INL_H_
#define MAIDSAFE_VAULT_PMID_NODE_SERVICE_INL_H_

#include <exception>
#include <string>

#include "maidsafe/common/log.h"

//#include "maidsafe/vault/utils.h"
#include "maidsafe/vault/pmid_manager/pmid_manager.pb.h"


namespace maidsafe {

namespace vault {

//template<typename Data>
//void PmidNodeService::HandleMessage(const nfs::Message& message,
//                                      const routing::ReplyFunctor& reply_functor) {
//  nfs::Reply reply(CommonErrors::success);
//  {
//    std::lock_guard<std::mutex> lock(accumulator_mutex_);
//    if (accumulator_.CheckHandled(message, reply))
//      return reply_functor(reply.Serialise()->string());
//  }

//  switch (message.data().action) {
//    case nfs::MessageAction::kPut:
//      return HandlePutMessage<Data>(message, reply_functor);
//    case nfs::MessageAction::kGet:
//      return HandleGetMessage<Data>(message, reply_functor);
//    case nfs::MessageAction::kDelete:
//      return HandleDeleteMessage<Data>(message, reply_functor);
//    default: {
//      reply = nfs::Reply(VaultErrors::operation_not_supported, message.Serialise().data);
//      std::lock_guard<std::mutex> lock(accumulator_mutex_);
//      accumulator_.SetHandled(message, reply);
//      reply_functor(reply.Serialise()->string());
//    }
//  }
//}

//template<typename Data>
//void PmidNodeService::HandlePutMessage(const nfs::Message& message,
//                                       const routing::ReplyFunctor& reply_functor) {
//  try {
//#ifndef TESTING
//    ValidatePutSender(message);
//#endif
//    Data data(typename Data::Name(message.data().name),
//              typename Data::serialised_type(message.data().content));
//    if (detail::AddResult(message, reply_functor, MakeError(CommonErrors::success),
//                          accumulator_, accumulator_mutex_, kPutRequestsRequired)) {
//      permanent_data_store_.Put(data.name(), message.data().content);
//    }
//  }
//  catch(const maidsafe_error& error) {
//    detail::AddResult(message, reply_functor, error, accumulator_, accumulator_mutex_,
//                      kPutRequestsRequired);
//  }
//  catch(...) {
//    detail::AddResult(message, reply_functor, MakeError(CommonErrors::unknown),
//                      accumulator_, accumulator_mutex_, kPutRequestsRequired);
//  }
//}

//template<typename Data>
//void PmidNodeService::HandleGetMessage(const nfs::Message& message,
//                                       const routing::ReplyFunctor& reply_functor) {
//  try {
//#ifndef TESTING
//    ValidateGetSender(message);
//#endif
//    typename Data::Name data_name(message.data().name);
//    nfs::Reply reply(CommonErrors::success, permanent_data_store_.Get(data_name));
//    reply_functor(reply.Serialise()->string());
//  } catch(const std::exception& /*ex*/) {
//    reply_functor(nfs::Reply(CommonErrors::unknown,
//                             message.Serialise().data).Serialise()->string());
//  }
//}

//template<typename Data>
//void PmidNodeService::HandleDeleteMessage(const nfs::Message& message,
//                                          const routing::ReplyFunctor& reply_functor) {
//  try {
//#ifndef TESTING
//    ValidateDeleteSender(message);
//#endif
//    if (detail::AddResult(message, reply_functor, MakeError(CommonErrors::success),
//                          accumulator_, accumulator_mutex_, kDeleteRequestsRequired)) {
//      permanent_data_store_.Delete(typename Data::Name(message.data().name));
//    }
//  }
//  catch(const maidsafe_error& error) {
//    detail::AddResult(message, reply_functor, error, accumulator_, accumulator_mutex_,
//                      kDeleteRequestsRequired);
//  }
//  catch(...) {
//    detail::AddResult(message, reply_functor, MakeError(CommonErrors::unknown),
//                      accumulator_, accumulator_mutex_, kDeleteRequestsRequired);
//  }
//}

template<>
void PmidNodeService::HandleMessage<nfs::GetRequestFromDataManagerToPmidNode>(
    const nfs::GetRequestFromDataManagerToPmidNode& message,
    const typename nfs::GetRequestFromDataManagerToPmidNode::Sender& sender,
    const typename nfs::GetRequestFromDataManagerToPmidNode::Receiver& /*receiver*/) {
  typedef nfs::GetResponseFromPmidNodeToDataManager NfsMessage;
  typedef routing::Message<NfsMessage::Sender, NfsMessage::Receiver> RoutingMessage;
  try {
#ifndef TESTING
    ValidateGetSender(sender);
#endif
    auto data_name(nfs::DataName(message.Contents));
    auto content(permanent_data_store_.Get(data_name));
    NfsMessage nfs_message(nfs::DataNameAndContent(data_name.type, data_name.raw_name, content));
    RoutingMessage message(nfs_message.Serialise(),
                           NfsMessage::Sender(routing::SingleId(routing_.KnodeId())),
                           sender);
    routing_.Send(message);
    {
      std::lock_guard<mutex> lock(accumulator_mutex_);
      accumulator_.SetHandled(message, sender);
    }
  } catch(const std::exception& /*ex*/) {
    // TODO(Mahmoud): Handle non-existing data or failure
  }
}

template<>
void PmidNodeService::HandleMessage<nfs::DeleteRequestFromPmidManagerToPmidNode>(
    const nfs::DeleteRequestFromPmidManagerToPmidNode& message,
    const typename nfs::DeleteRequestFromPmidManagerToPmidNode::Sender& sender,
    const typename nfs::DeleteRequestFromPmidManagerToPmidNode::Receiver& /*receiver*/) {
  try {
#ifndef TESTNG
  ValidateDeleteSender(sender);
#endif
    {
      std::lock_guard<mutex> lock(accumulator_mutex_);
      if (!accumulator_.AddPendingRequest(message, sender, kDeleteRequestsRequired))
        return;
      permanent_data_store_.Delete(nfs::DataName(message.Contents));
      accumulator_.SetHandled(message, sender);
    }
  } catch(const std::exception& /*ex*/) {
      // TODO(Mahmoud): Handle failure to delete
  }
}

template<>
void PmidNodeService::HandleMessage<nfs::PutRequestFromPmidManagerToPmidNode>(
    const nfs::PutRequestFromPmidManagerToPmidNode& message,
    const typename nfs::PutRequestFromPmidManagerToPmidNode::Sender& sender,
    const typename nfs::PutRequestFromPmidManagerToPmidNode::Receiver& /*receiver*/) {
  try {
#ifndef TESTNG
  ValidateDeleteSender(sender);
#endif
    {
      std::lock_guard<mutex> lock(accumulator_mutex_);
      if (!accumulator_.AddPendingRequest(message, sender, kDeleteRequestsRequired))
        return;
      permanent_data_store_.Put(nfs::DataName(message.Contents.name), message.Contents.content);
      accumulator_.SetHandled(message, sender);
    }
  } catch(const std::exception& /*ex*/) {
      // TODO(Mahmoud): Handle failure to put
  }
}

template<>
bool PmidNodeService::GetFromCache<nfs::GetRequestFromMaidNodeToDataManager>(
    const nfs::GetRequestFromMaidNodeToDataManager& message,
    const typename nfs::GetRequestFromMaidNodeToDataManager::Sender& sender,
    const typename nfs::GetRequestFromMaidNodeToDataManager::Receiver& receiver) {
    return GetFromCache(message, sender, receiver,
                        is_cacheable<nfs::DataName(message.Contents>()));
}

template<>
bool GetFromCache<nfs::GetRequestFromPmidNodeToDataManager>(
    const nfs::GetRequestFromPmidNodeToDataManager& message,
    const typename nfs::GetRequestFromPmidNodeToDataManager::Sender& sender,
    const typename nfs::GetRequestFromPmidNodeToDataManager::Receiver& receiver) {
  return GetFromCache(message, sender, receiver,
                      is_cacheable<nfs::DataName(message.Contents>()));
}


template<typename T>
bool PmidNodeService::GetFromCache(const T& message,
                                   const typename T::Sender& sender,
                                   const typename T::Receiver& receiver,
                                   IsCacheable) {
  return CacheGet<T>(message,
                     sender,
                     receiver,
                     is_long_term_cachable<nfs::DataName(message.Contents())>);
}

template<typename T>
bool PmidNodeService::GetFromCache(const T& /*message*/,
                                   const typename T::Sender& /*sender*/,
                                   const typename T::Receiver& /*receiver*/,
                                   IsNotCacheable) {
  return false;
}


template<typename T>
bool PmidNodeService::CacheGet(const T& message,
                               const typename T::Sender& sender,
                               const typename T::Receiver& receiver,
                               IsShortTermCacheable) {
  static_assert(is_short_term_cacheable<nfs::Dataname(message.Contents())>::value,
                "This should only be called for short-term cacheable data types.");
  NonEmptyString data(mem_only_cache_.Get(message.Contents()));
  if (data.empty())
    return false;
  // TODO(Mahmoud): Must send the data to the requestor
}

template<typename T>
NonEmptyString PmidNodeService::CacheGet(const T& message,
                                         const typename T::Sender& sender,
                                         const typename T::Receiver& receiver,
                                         IsLongTermCacheable) {
  static_assert(is_long_term_cacheable<nfs::Dataname(message.Contents())>::value,
                "This should only be called for long-term cacheable data types.");
  NonEmptyString data(cache_data_store_.Get(message.Contents()));
  if (data.empty())
    return false;
  // TODO(Mahmoud): Must send the data to the requestor
}

template<>
void PmidNodeService::StoreInCache<nfs::GetResponseFromDataManagerToMaidNode>(
    const nfs::GetResponseFromDataManagerToMaidNode& message,
    const typename nfs::GetResponseFromDataManagerToMaidNode::Sender& /*sender*/,
    const typename nfs::GetResponseFromDataManagerToMaidNode::Receiver& /*receiver*/) {
  StoreInCache(message, is_cacheable<Data>(nfs::DataName(message.Contents().data.name)));
}


template<typename T>
void PmidNodeService::StoreInCache(const T& message, IsCacheable) {
  CacheStore(message, is_long_term_cacheable<nfs::DataName(message.Contents.data.name>()));
}

template<typename T>
void PmidNodeService::StoreInCache(const T& /*message*/, IsNotCacheable) {}

template<typename T>
void PmidNodeService::CacheStore(const T& message, IsShortTermCacheable) {
  static_assert(is_short_term_cacheable<nfs::DataName(message.Contents.data.name)>::value,
                "This should only be called for short-term cacheable data types.");
  return mem_only_cache_.Store(message.Contents.data.name, message.Contents.data.value);
}

template<typename T>
void PmidNodeService::CacheStore(const T& message, IsLongTermCacheable) {
  static_assert(is_long_term_cacheable<nfs::DataName(message.Contents.data.name)>::value,
                "This should only be called for long-term cacheable data types.");
  return cache_data_store_.Store(message.Contents.data.name, message.Contents.data.value);
}

}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_PMID_NODE_SERVICE_INL_H_
