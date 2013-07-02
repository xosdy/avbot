
/*
 * Copyright (C) 2012 - 2013  微蔡 <microcai@fedoraproject.org>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#pragma once

#ifndef __AVHTTP_MISC_HTTP_READBODY_HPP__
#define __AVHTTP_MISC_HTTP_READBODY_HPP__

#include <boost/shared_ptr.hpp>
#include <boost/function.hpp>
#include <boost/asio.hpp>
#include <avhttp.hpp>

#include <boost/asio/yield.hpp>

namespace avhttp{
namespace misc{


namespace detail{

// match condition!
static inline std::size_t async_http_download_condition(boost::system::error_code ec, std::size_t bytes_transferred, std::string content_length)
{
	if (ec)
		return 0;

	if ( content_length.empty() ){
		return 4096;
	}else{
		// 读取到 content_length 是吧.
		return boost::lexical_cast<std::size_t>( content_length ) - bytes_transferred;
	}
}

template<class avHttpStream, class MutableBufferSequence, class Handler>
class async_read_body_op : boost::asio::coroutine {
public:
	typedef void result_type;
public:
	async_read_body_op( avHttpStream &_stream, const avhttp::url & url, MutableBufferSequence &_buffers, Handler _handler )
		: m_handler( _handler ), m_stream( _stream ), m_buffers(_buffers)
	{
		m_stream.async_open( url, *this );
	}

	void operator()( const boost::system::error_code& ec , std::size_t length = 0 ) {
		reenter( this )
		{
			if( !ec ) {
				content_length = m_stream.response_options().find( avhttp::http_options::content_length );

				yield boost::asio::async_read(m_stream, m_buffers, boost::bind(async_http_download_condition, _1 , _2, content_length), *this );
			}

			if (ec == boost::asio::error::eof &&  !content_length.empty() && length == boost::lexical_cast<std::size_t>( content_length ) )
			{
				m_handler(boost::system::error_code(), length);
			}else{
				m_handler( ec, length);
			}
		}
	}

private:
	std::string content_length;
	avHttpStream & m_stream;
	Handler m_handler;
	MutableBufferSequence &m_buffers;
};

template<class avHttpStream, class MutableBufferSequence, class Handler>
async_read_body_op<avHttpStream, MutableBufferSequence, Handler> make_async_read_body_op(avHttpStream & stream, const avhttp::url & url, MutableBufferSequence &buffers, Handler _handler)
{
	return async_read_body_op<avHttpStream, MutableBufferSequence, Handler>(stream, url, buffers, _handler);
}

} // namespace detail

template<class avHttpStream, class MutableBufferSequence, class Handler>
void async_read_body(avHttpStream & stream, const avhttp::url & url, MutableBufferSequence &buffers, Handler _handler)
{
	detail::make_async_read_body_op(stream, url, buffers, _handler);
}

} // namespace misc
} // namespace avhttp

#endif // __AVHTTP_MISC_HTTP_READBODY_HPP__
