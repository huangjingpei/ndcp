#ifndef __NDCP_INSTANCE_H__ 
#define __NDCP_INSTANCE_H__

#pragma once
#ifndef __TUYA_EXPORT_H__
#define __TUYA_EXPORT_H__ST
namespace tuya {
#ifdef WIN32
#ifdef TUYARTC_STATICLIB
#define SDK_EXPORT
#else  // TUYARTC_STATICLIB
#ifdef SDKWINDOWS_EXPORTS
#define SDK_EXPORT __declspec(dllexport)
#else  // SDKWINDOWS_EXPORTS
#define SDK_EXPORT __declspec(dllimport)
#endif  // SDKWINDOWS_EXPORTS
#endif  // TUYARTC_STATICLIB
#else   // WIN32

#if __has_attribute(visibility) && defined(SDKWINDOWS_EXPORTS)
#define SDK_EXPORT __attribute__((visibility("default")))
#endif

#endif  // WIN32

#ifndef SDK_EXPORT
#define SDK_EXPORT
#endif  // SDK_EXPORT
}  // namespace tuya
#endif  //__TUYA_EXPORT_H__

class NdcpInstance {

};
#endif  //__NDCP_INSTANCE_H__