// ���� ifdef ���Ǵ���ʹ�� DLL �������򵥵�
// ��ı�׼�������� DLL �е������ļ��������������϶���� WINSOCKET_EXPORT
// ���ű���ġ���ʹ�ô� DLL ��
// �κ�������Ŀ�ϲ�Ӧ����˷��š�������Դ�ļ��а������ļ����κ�������Ŀ���Ὣ
// WINSOCKET_API ������Ϊ�� DLL ���룬���� DLL ���ô˺궨���
// ������Ϊ�Ǳ������ġ�
#ifdef WINSOCKET_EXPORTS
#define WINSOCKET_API __declspec(dllexport)
#else
#define WINSOCKET_API __declspec(dllimport)
#endif

// ���ർ���� WinSocket.dll
class WINSOCKET_API CWinSocket {
public:
	CWinSocket(void);
	// TODO:  �ڴ�������ķ�����
};

extern WINSOCKET_API int nWinSocket;

WINSOCKET_API int fnWinSocket(void);