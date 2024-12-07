//
// Created by I Gede Panca Sutresna on 07/12/24.
//

#ifndef NUANSA_UTILS_LOG_LOG_H
#define NUANSA_UTILS_LOG_LOG_H

#define LOG_ERROR BOOST_LOG_TRIVIAL(error) << "[" << __FILE__ << ":" << __LINE__ << " " << __func__ << "] "
#define LOG_WARNING BOOST_LOG_TRIVIAL(warning) << "[" << __FILE__ << ":" << __LINE__ << " " << __func__ << "] "
#define LOG_INFO BOOST_LOG_TRIVIAL(info) << "[" << __FILE__ << ":" << __LINE__ << " " << __func__ << "] "
#define LOG_DEBUG BOOST_LOG_TRIVIAL(debug) << "[" << __FILE__ << ":" << __LINE__ << " " << __func__ << "] "

#endif //NUANSA_UTILS_LOG_LOG_H
