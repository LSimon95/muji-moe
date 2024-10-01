/**
 * Copyright(c) 2024 Reecho inc. All rights reserved.
 */
#include "window.hpp"
#include "statebar.hpp"
#include "texManager.hpp"
#include "../plat.hpp"
#include <map>
#include <iostream>
#include "../server/chat.hpp"

CStateBar::CStateBar()
{
	m_lastMousePressed = false;
}

CStateBar::~CStateBar()
{
	CWindow *window = CWindow::GetInstance();
	CTexManager *texManager = window->GetTexManager();
}

void CStateBar::Initialize()
{
	CWindow *window = CWindow::GetInstance();
	CTexManager *texManager = window->GetTexManager();
	auto executeAbsolutePath = window->GetExecuteAbsolutePath();

	auto programId = window->GetBaseShader();
	glUseProgram(programId);

	m_positionLocation = glGetAttribLocation(programId, "position");
	m_uvLocation = glGetAttribLocation(programId, "uv");
	m_textureLocation = glGetUniformLocation(programId, "texture");
	m_colorLocation = glGetUniformLocation(programId, "baseColor");

	Csm::CubismVector2 bkPos = Csm::CubismVector2(613.406f, -439.0577f);

	std::map<std::string, Csm::CubismVector2> controlOffsets = {
		{"bk", Csm::CubismVector2(0.0f, 0.0f)},
		{"call", Csm::CubismVector2(527.3378f, -439.0577f) - bkPos},
		{"close", Csm::CubismVector2(867.2645f, -439.0577f) - bkPos},
		{"menu", Csm::CubismVector2(694.1955f, -439.0577f) - bkPos},
		{"mv", Csm::CubismVector2(352.9271f, -439.0577f) - bkPos},
	};

	m_barScale = 0.2f;
	for (auto const &offset : controlOffsets)
	{
		Control control;
		control.name = offset.first;
		control.tex = texManager->CreateTextureFromPngFile(executeAbsolutePath + "/Resources/statebar/" + control.name + ".png", true);
		control.offset = offset.second;
		control.pressed = false;
		control.opened = false;
		m_controls.push_back(control);

		if (control.name == "bk")
		{
			m_barZoneHeight = control.tex->height * 2 * m_barScale;
		}
	}
}

void CStateBar::confirmCloseCallback()
{
	glfwSetWindowShouldClose(CWindow::GetInstance()->GetGLWindow(), GLFW_TRUE);
}

void CStateBar::clickCallback(Control *control)
{
	auto world = CWorld::GetInstance();
	auto window = world->GetWindow();
	if (control->name == "mv")
	{
		CPlat::GetCursorPos(&m_initMouseX, &m_initMouseY);
		glfwGetWindowPos(window->GetGLWindow(), &m_initWinX, &m_initWinY);
	}
	else if (control->name == "menu")
	{
		world->m_showPanel = !world->m_showPanel;
	}
	else if (control->name == "close")
	{
		window->SetMessage(
			world->T("Are you sure you want to close the application?"),
			std::bind(&CStateBar::confirmCloseCallback, this));
	}
}

void CStateBar::pressedCallback(Control *control)
{
	if (control->name == "mv")
	{
		float x, y;
		CPlat::GetCursorPos(&x, &y);
		glfwSetWindowPos(CWindow::GetInstance()->GetGLWindow(), m_initWinX + (x - m_initMouseX), m_initWinY + (y - m_initMouseY));
	}
}

void CStateBar::RenderPic(
	CStateBar::Control *control,
	int winWidth, int winHeight,
	Csm::CubismVector2 &offset,
	float mouseX, float mouseY,
	bool mousePressed)
{
	float spriteColor[4] = {1.0f, 1.0f, 1.0f, 1.0f};
	const GLfloat uvVertex[] =
		{
			1.0f,
			1.0f,
			0.0f,
			1.0f,
			0.0f,
			0.0f,
			1.0f,
			0.0f,
		};

	auto tex = control->tex;

	glEnableVertexAttribArray(m_positionLocation);
	glEnableVertexAttribArray(m_uvLocation);

	glUniform1i(m_textureLocation, 0);

	float widthScreen = (float)tex->width / (float)(winWidth) * 2.0f;
	float heightScreen = (float)tex->height / (float)(winHeight) * 2.0f;

	auto controlOffset = control->offset;
	controlOffset.X = controlOffset.X / (float)(winWidth) * 2.0f;
	controlOffset.Y = controlOffset.Y / (float)(winHeight) * 2.0f;

	Csm::CubismVector2 positionVertex[] = {
		(Csm::CubismVector2(widthScreen * 0.5, -heightScreen * 0.5) + controlOffset) * m_barScale + offset,
		(Csm::CubismVector2(-widthScreen * 0.5, -heightScreen * 0.5) + controlOffset) * m_barScale + offset,
		(Csm::CubismVector2(-widthScreen * 0.5, heightScreen * 0.5) + controlOffset) * m_barScale + offset,
		(Csm::CubismVector2(widthScreen * 0.5, heightScreen * 0.5) + controlOffset) * m_barScale + offset};


	auto world = CWorld::GetInstance();
	if (control->name == "call")
	{
		if (world->GetChatServerIsRunning())
		{
			spriteColor[0] = 0.15234375f;
			spriteColor[1] = 0.44140625f;
			spriteColor[2] = 0.17578125f;
		}
		else
		{
			spriteColor[0] = 0.44140625f;
			spriteColor[1] = 0.15234375f;
			spriteColor[2] = 0.17578125f;
		}
	}

	if (control->name != "bk" && !control->pressed &&
		mouseX > positionVertex[1].X && mouseX < positionVertex[0].X &&
		mouseY < positionVertex[2].Y && mouseY > positionVertex[1].Y)
	{
		if (control->name == "close")
		{
			spriteColor[0] = 2.0f;
		}
		else if (control->name == "call")
		{
			if (world->GetChatServerIsRunning())
				spriteColor[1] = spriteColor[1] * 2.0f;
			else
				spriteColor[0] = spriteColor[0] * 2.0f;
		}
		else
		{
			spriteColor[0] = 0.4f;
			spriteColor[1] = 0.7255f;
			spriteColor[2] = 0.7490f;
		}

		if (mousePressed && !m_lastMousePressed)
		{
			control->pressed = true;
			clickCallback(control);
		}
	}
	else if (control->pressed || control->opened)
	{
		spriteColor[0] = 0.0274f;
		spriteColor[1] = 0.5333f;
		spriteColor[2] = 0.60784;

		pressedCallback(control);
		if (!mousePressed)
		{
			if (control->pressed)
			{
				if (control->name == "call" && !world->GetChatServerIsRunning())
					world->GetChat()->SendCommand2Chat(CChat::SChatCommand{CChat::EChatCommand::CHAT_COMMAND_RELOAD_CONFIG});
			}

			control->pressed = false;
		}
	}

	glVertexAttribPointer(m_positionLocation, 2, GL_FLOAT, false, 0, positionVertex);
	glVertexAttribPointer(m_uvLocation, 2, GL_FLOAT, false, 0, uvVertex);

	glUniform4f(m_colorLocation, spriteColor[0], spriteColor[1], spriteColor[2], spriteColor[3]);

	glBindTexture(GL_TEXTURE_2D, tex->id);
	glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
}

void CStateBar::Render()
{
	auto world = CWorld::GetInstance();
	auto window = world->GetWindow();
	int width, height;
	glfwGetWindowSize(window->GetGLWindow(), &width, &height);
	Csm::CubismVector2 offset = Csm::CubismVector2(world->m_showPanel ? -0.5f : 0.0f, ((float)m_barZoneHeight / 2 / (float)height * 2.0f) - 1.0f);

	float mouseX = window->GetMouseWinX(), mouseY = window->GetMouseWinY();
	bool mousePressed = window->GetMousePressed();

	for (int i = 0; i < m_controls.size(); i++)
	{
		if (m_controls[i].name == "menu")
		{
			m_controls[i].opened = world->m_showPanel;
		}
		RenderPic(&m_controls[i], width, height, offset, mouseX, mouseY, mousePressed);
	}

	m_lastMousePressed = mousePressed;
}