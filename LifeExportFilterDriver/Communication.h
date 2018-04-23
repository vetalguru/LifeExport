#ifndef _LIFE_EXPORT_COMMUNICATION_H_
#define _LIFE_EXPORT_COMMUNICATION_H_



// Server port names
#define AA_CREATE_PORT_NAME		L"\\LifeExportFilterCreatePort"



// Possible connection types
typedef enum _LIFE_EXPORT_CONNECTION_TYPE
{
	LIFE_EXPORT_CREATE_CONNECTION_TYPE = 1  // Zero means empty (wrong) value
}LIFE_EXPORT_CONNECTION_TYPE, *PLIFE_EXPORT_CONNECTION_TYPE;



#endif // _LIFE_EXPORT_COMMUNICATION_H_
