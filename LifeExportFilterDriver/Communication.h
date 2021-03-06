#ifndef _LIFE_EXPORT_COMMUNICATION_H_
#define _LIFE_EXPORT_COMMUNICATION_H_


// Filter driver name
#define FILTER_DRIVER_NAME      L"LifeExportFilterDriver"


// Server port names
#define AA_CREATE_PORT_NAME     L"\\LifeExportFilterCreatePort"
#define AA_READ_PORT_NAME       L"\\LifeExportFilterReadPort"
#define AA_CONTROL_PORT_NAME    L"\\LifeExportFilterControlPort"


// Possible connection types
typedef enum _LIFE_EXPORT_CONNECTION_TYPE
{
    LIFE_EXPORT_CREATE_CONNECTION_TYPE = 1,  // Zero means empty (wrong) value
    LIFE_EXPORT_READ_CONNECTION_TYPE,
    LIFE_EXPORT_CONTROL_CONNECTION_TYPE,

} LIFE_EXPORT_CONNECTION_TYPE, *PLIFE_EXPORT_CONNECTION_TYPE;


typedef union _AA_FILE_ID_INFO
{
    struct
    {
        ULONGLONG Value;
        ULONGLONG UpperZeroes;
    } FileId_64;

    FILE_ID_128 FileId_128;

} AA_FILE_ID_INFO, *PAA_FILE_ID_INFO;


#define AA_INVALID_FILE_REFERENCE( _fileId_ ) \
    (((_fileId_).FileId_64.UpperZeroes == 0ll) && \
     ((_fileId_).FileId_64.Value == (ULONGLONG)FILE_INVALID_FILE_ID))


#define AA_SET_INVALID_FILE_REFERENCE( _fileId_ ) \
      (_fileId_).FileId_64.UpperZeroes = 0ll;\
      (_fileId_).FileId_64.Value = (ULONGLONG)FILE_INVALID_FILE_ID


// Connecting
typedef enum _LIFE_EXPORT_CONNECTION_RESULT
{
    CREATE_RESULT_SKIP_FILE = 1,
    CREATE_RESULT_EXPORT_FILE

} LIFE_EXPORT_CONNECTION_RESULT, *PLIFE_EXPORT_CONNECTION_RESULT;


typedef struct _LIFE_EXPORT_CREATE_NOTIFICATION_REQUEST
{
    AA_FILE_ID_INFO FileId;

} LIFE_EXPORT_CREATE_NOTIFICATION_REQUEST, *PLIFE_EXPORT_CREATE_NOTIFICATION_REQUEST;

typedef struct _LIFE_EXPORT_CREATE_NOTIFICATION_RESPONSE
{
    AA_FILE_ID_INFO               FileId;
    LIFE_EXPORT_CONNECTION_RESULT ConnectionResult;

} LIFE_EXPORT_CREATE_NOTIFICATION_RESPONSE, *PLIFE_EXPORT_CREATE_NOTIFICATION_RESPONSE;


typedef struct _LIFE_EXPORT_CONNECTION_CONTEXT
{
    LIFE_EXPORT_CONNECTION_TYPE Type;

} LIFE_EXPORT_CONNECTION_CONTEXT, *PLIFE_EXPORT_CONNECTION_CONTEXT;


// Reading
typedef enum _LIFE_EXPORT_READ_RESULT_ACTION
{
    SUCCESS_WITH_NO_POST_CALLBACK = 1, // FLT_PREOP_SUCCESS_NO_CALLBACK
    SUCCESS_WITH_POST_CALLBACK,        // FLT_PREOP_SUCCESS_WITH_CALLBACK
    COMPLETE_WITH_NO_POST_CALLBACK,    // FLT_PREOP_COMPLETE
} LIFE_EXPORT_READ_RESULT_ACTION, *PLIFE_EXPORT_READ_RESULT_ACTION;

typedef enum _LIFE_EXPORT_READ_NOTIFICATION_TYPE
{
    READ_NOTIFICATION_PRE_READ_TYPE = 1,
    READ_NOTIFICATION_POST_READ_TYPE,
} LIFE_EXPORT_READ_NOTIFICATION_TYPE, *PLIFE_EXPORT_READ_NOTIFICATION_TYPE;

typedef struct _LIFE_EXPORT_USER_BUFFER
{
    PVOID BufferPtr;
    ULONG BufferSize;
    ULONG CurrentProcessId;
} LIFE_EXPORT_USER_BUFFER, *PLIFE_EXPORT_USER_BUFFER;


typedef struct _LIFE_EXPORT_READ_NOTIFICATION_REQUEST
{
    LIFE_EXPORT_READ_NOTIFICATION_TYPE  RequestType;
    AA_FILE_ID_INFO                     FileId;
    ULONGLONG                           BlockFileOffset;
    ULONGLONG                           BlockLength;
    LIFE_EXPORT_USER_BUFFER             UserBuffer;
    NTSTATUS                            Status;
} LIFE_EXPORT_READ_NOTIFICATION_REQUEST, *PLIFE_EXPORT_READ_NOTIFICATION_REQUEST;


typedef struct _LIFE_EXPORT_READ_NOTIFICATION_RESPONSE
{
    AA_FILE_ID_INFO                FileId;
    LIFE_EXPORT_READ_RESULT_ACTION ReadResultAction;
    LIFE_EXPORT_USER_BUFFER        UserBuffer;
} LIFE_EXPORT_READ_NOTIFICATION_RESPONSE, *PLIFE_EXPORT_READ_NOTIFICATION_RESPONSE;


// Control
typedef enum _LIFE_EXPORT_CONTROL_MESSAGE
{
    CONTROL_RESULT_UNLOADING = 1,

} LIFE_EXPORT_CONTROL_MESSAGE, *PLIFE_EXPORT_CONTROL_MESSAGE;

typedef enum _LIFE_EXPORT_CONTROL_RESULT
{
    CONTROL_RESULT_UNLOAD = 1,

} LIFE_EXPORT_CONTROL_RESULT, *PLIFE_EXPORT_CONTROL_RESULT;


typedef struct _LIFE_EXPORT_CONTROL_NOTIFICATION_REQUEST
{
    LIFE_EXPORT_CONTROL_MESSAGE Message;

} LIFE_EXPORT_CONTROL_NOTIFICATION_REQUEST, *PLIFE_EXPORT_CONTROL_NOTIFICATION_REQUEST;


typedef struct _LIFE_EXPORT_CONTROL_NOTIFICATION_RESPONSE
{
    LIFE_EXPORT_CONTROL_RESULT ConnectionResult;

} LIFE_EXPORT_CONTROL_NOTIFICATION_RESPONSE, *PLIFE_EXPORT_CONTROL_NOTIFICATION_RESPONSE;


#endif // _LIFE_EXPORT_COMMUNICATION_H_

