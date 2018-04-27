#ifndef _LIFE_EXPORT_COMMUNICATION_H_
#define _LIFE_EXPORT_COMMUNICATION_H_



// Server port names
#define AA_CREATE_PORT_NAME		L"\\LifeExportFilterCreatePort"


// Possible connection types
typedef enum _LIFE_EXPORT_CONNECTION_TYPE
{
	LIFE_EXPORT_CREATE_CONNECTION_TYPE = 1  // Zero means empty (wrong) value

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
	AA_FILE_ID_INFO FileId;

	LIFE_EXPORT_CONNECTION_RESULT ConnectionResult;

} LIFE_EXPORT_CREATE_NOTIFICATION_RESPONSE, *PLIFE_EXPORT_CREATE_NOTIFICATION_RESPONSE;


typedef struct _LIFE_EXPORT_CONNECTION_CONTEXT
{
	LIFE_EXPORT_CONNECTION_TYPE Type;

} LIFE_EXPORT_CONNECTION_CONTEXT, *PLIFE_EXPORT_CONNECTION_CONTEXT;


#endif // _LIFE_EXPORT_COMMUNICATION_H_
