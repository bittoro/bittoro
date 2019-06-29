// Copyright (c) 2006-2013, Andrey N. Sabelnikov, www.sabelnikov.net
// All rights reserved.
// 
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
// * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
// * Redistributions in binary form must reproduce the above copyright
// notice, this list of conditions and the following disclaimer in the
// documentation and/or other materials provided with the distribution.
// * Neither the name of the Andrey N. Sabelnikov nor the
// names of its contributors may be used to endorse or promote products
// derived from this software without specific prior written permission.
// 
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER  BE LIABLE FOR ANY
// DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
// LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
// ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
// 


#pragma once

#include <string>

namespace epee
{
  namespace net_utils
  {
    inline
    bool is_ip_local(uint32_t ip)
    {
      /*
      private network ranges:
      10.0.0.0/8
      172.16.0.0/12
      192.168.0.0/16

      carrier-grade NAT network range:
      100.64.0.0/10

      link-local addresses:
      169.254.0.0/16
      */

      // Extremely bizarrely, IPs are stored in little-endian order in epee, which is just plain
      // wrong, but we have to deal with.  (Jason)
      if ((ip & 0xff) == 0x0a) // 10.0.0.0/8
        return true;

      if ((ip & 0xf0ff) == 0x10ac) // 172.16.0.0/12 (0xf0ff looks strange because of the little endian nonsense)
        return true;

      if ((ip & 0xffff) == 0xa8c0) // 192.168.0.0/16
        return true;

      if ((ip & 0xc0ff) == 0x4064) // 100.64.0.0/10
        return true;

      if ((ip & 0xffff) == 0xfea9) // 169.254.0.0/16
        return true;

      return false;
    }
    inline
    bool is_ip_loopback(uint32_t ip)
    {
      if ((ip & 0xff) == 0x7f) // 127.0.0.0/8
        return true;

      return false;
    }

    inline
    bool is_ip_public(uint32_t ip)
    {
      return !(
          is_ip_local(ip) ||
          is_ip_loopback(ip) ||
          (ip & 0xff) == 0x00 || // 0.0.0.0/8 addresses are "current network" addresses valid only as a source but not destination
          (ip & 0xe0) == 0xe0); // 224.0.0.0/3 -- includes both 224/4 multicast and 240/4 reserved blocks
    }

    inline
    bool is_ipv6_local(const std::string& ip)
    {
      // ipv6 local addresses start with fc00/7 -- (fcXX or fdXX)
      return (ip.find("fc") == 0 || ip.find("FC") == 0 || ip.find("fd") == 0 || ip.find("FD") == 0);
    }

    inline
    bool is_ipv6_loopback(const std::string& ip)
    {
      // ipv6 loopback is ::1
      return (ip == "::1");
    }
   
  }
}

