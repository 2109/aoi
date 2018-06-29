
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
	glFlush();  //执行OpenGL指令列表中的指令
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

	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);    //设置显示模式
	glutInitWindowSize(width, height); //设置窗口大小
	glutInitWindowPosition(200, 200);   //设置窗口在屏幕上的位置
	glutCreateWindow("simple aoi"); //创建窗口并给出标题
	glutDisplayFunc(renderScene);   //注册显示窗口时回调函数renderScene
	glClearColor(1.0f, 1.0f, 1.0f, 1.0f);   //使用蓝色清空背景底
	glOrtho(0.0f, width, 0.0f, height, 1.0, -1.0);

	glutTimerFunc(33, TimerFunc, 1);
	glutMainLoop();     //消息循环（处理操作系统等的消息，例如键盘、鼠标事件等）
	return 0;
}

