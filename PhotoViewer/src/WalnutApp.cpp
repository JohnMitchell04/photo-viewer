#include "Walnut/Application.h"
#include "Walnut/EntryPoint.h"

#include "Walnut/Image.h"

#include "Image.h"
#include "PNG.h"

class ExampleLayer : public Walnut::Layer
{
public:
	virtual void OnUIRender() override
	{
		ImGui::Begin("Control Panel");
		if (ImGui::Button("Open")) {
			m_loadedImage = std::make_unique<ImageLibrary::PNG>("C:\\Users\\johnr\\source\\repos\\photo-viewer\\PhotoViewer\\test\\2pixeltest.png");
		}
		ImGui::End();

		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
		ImGui::Begin("Viewport");

		// Use later when adding zoom
		/*
		m_ViewportWidth = ImGui::GetContentRegionAvail().x;
		m_ViewportHeight = ImGui::GetContentRegionAvail().y;
		*/

		if (m_loadedImage)
			ImGui::Image(m_loadedImage->GetDescriptorSet(), { (float)m_loadedImage->GetWidth(), (float)m_loadedImage->GetHeight() });

		ImGui::End();
		ImGui::PopStyleVar();
	}

private:
	std::unique_ptr<ImageLibrary::Image> m_loadedImage;
};

Walnut::Application* Walnut::CreateApplication(int argc, char** argv)
{
	Walnut::ApplicationSpecification spec;
	spec.Name = "Photo Viewer";

	Walnut::Application* app = new Walnut::Application(spec);
	app->PushLayer<ExampleLayer>();
	app->SetMenubarCallback([app]()
	{
		if (ImGui::BeginMenu("File"))
		{
			if (ImGui::MenuItem("Exit"))
			{
				app->Close();
			}
			ImGui::EndMenu();
		}
	});
	return app;
}