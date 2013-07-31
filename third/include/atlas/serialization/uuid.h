/*
 * serialization.h
 *
 *  Created on: Apr 8, 2011
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

#ifndef CLOWN_SERIALIZATION_UUID_H_
#define CLOWN_SERIALIZATION_UUID_H_

#include <boost/uuid/uuid.hpp>

namespace boost {
  namespace serialization {

    template<typename Archive>
    void serialize(Archive& ar, boost::uuids::uuid& uuid, const unsigned int version) {
      ar & uuid.data;
    }

  } // serialization
} // boost

#endif /* SERIALIZATION_H_ */
