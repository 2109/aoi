
#include "glut.h"
#include "Aoi.h"
#include "AoiEntity.h"
#include "AoiTrigger.h"
#include "AoiContext.h"
#include <vector>

AoiContext* g_context;

void renderScene(void)
{
	glClear(GL_COLOR_BUFFER_BIT);
	g_context->Draw();
	glFlush();  //ִ��OpenGLָ���б��е�ָ��
	glutSwapBuffers();
}

void idleFunc(void) {

}

void TimerFunc(int id) {
	glutTimerFunc(33, TimerFunc, id);
	g_context->Update(0.33);
	glutPostRedisplay();
}

int main()
{
	int width = 1024;
	int height = 500;

	AoiContext* context = new AoiContext(width, height);
	g_context = context;
	for ( int i = 0; i < 1000; i++ )
	{
		context->CreateEntity();
	}

	for ( int i = 0; i < 10; i++ )
	{
		context->CreateTrigger();;
	}

	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);    //������ʾģʽ
	glutInitWindowSize(width, height); //���ô��ڴ�С
	glutInitWindowPosition(200, 200);   //���ô�������Ļ�ϵ�λ��
	glutCreateWindow("simple aoi"); //�������ڲ���������
	glutDisplayFunc(renderScene);   //ע����ʾ����ʱ�ص�����renderScene
	glClearColor(1.0f, 1.0f, 1.0f, 1.0f);   //ʹ����ɫ��ձ�����
	glOrtho(0.0f, width, 0.0f, height, 1.0, -1.0);

	glutTimerFunc(33, TimerFunc, 1);
	glutMainLoop();     //��Ϣѭ�����������ϵͳ�ȵ���Ϣ��������̡�����¼��ȣ�
	return 0;
}

