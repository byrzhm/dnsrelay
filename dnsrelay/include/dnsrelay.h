#pragma once

/**
 * todo: dnsrelay �ӿ�, �������岿��
*/


#ifdef __cplucplus
extern "C" {
#endif

/**
 * @fn �����߳�, �׽���, ��ȡ�����ļ�
 * @brief �м̷������ĳ�ʼ��
 * @param argc �����в�������
 * @param argv �����в�������
*/
void dnsrelay_init(int argc, const char *argv[]);



/**
 * @brief ������ѭ��
*/
void main_loop();


#ifdef __cplucplus
}
#endif