��Ҫ�����ǽ���һ��h264����Ƶ, ����ʾ��panel/HDMI��ʾ�豸��, ֧��rotate���ܡ�
�÷����£�
	��ת��    ./SsPlayer 720P25.h264 0 0 480 320 1
	������ʾ�� ./SsPlayer 720P25.h264 0 0 1280 720 0
ע�����
	1.vdec���������������Ƶ�������, vdec��������disp����������һ�¼���;
	
��ԭ����������Ƶ�Ļ����������˲��������Ĺ��ܣ�
�����������£�
    ./SsPlayer 720P25.h264 0 0 480 320 1 48k_stereo.wav 2 48000
	
Ĭ��ʹ��panel��ʾ, ����enable hdmi��ʾ, ����Makefile, ENABLE_HDMI = 0 => ENABLE_HDMI = 1
	
ע��demo��ʹ��ffmpeg������demux h264�ļ�, ��ȡ��00 00 00 01 NALͷ������֡�͵�VDEC����. ��Makefile��ʹ�� USE_FFMPEG = 1