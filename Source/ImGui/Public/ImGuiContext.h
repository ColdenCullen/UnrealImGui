#pragma once

#include <Engine/Texture2D.h> // TStrongObjectPtr doesn't like forward declared types
#include <Templates/SharedPointer.h>
#include <UObject/StrongObjectPtr.h>

class SWindow;
class SImGuiOverlay;
struct FDisplayMetrics;
struct ImGuiContext;
struct ImGuiViewport;
struct ImPlotContext;

struct IMGUI_API FImGuiViewportData
{
	/// Returns the existing viewport data or creates one
	static FImGuiViewportData* GetOrCreate(ImGuiViewport* Viewport);

	TWeakPtr<SWindow> Window = nullptr;
	TWeakPtr<SImGuiOverlay> Overlay = nullptr;
};

class IMGUI_API FImGuiContext : public TSharedFromThis<FImGuiContext>
{
public:
	/// Creates a managed ImGui context
	static TSharedRef<FImGuiContext> Create();

	/// Returns an existing managed ImGui context
	static TSharedPtr<FImGuiContext> Get(ImGuiContext* Context);

	~FImGuiContext();

	/// Implicit conversion operator for the underlying ImGui context
	operator ImGuiContext*() const;

	/// Implicit conversion operator for the underlying ImPlot context
	operator ImPlotContext*() const;

private:
	void Initialize();

	void OnDisplayMetricsChanged(const FDisplayMetrics& DisplayMetrics) const;
	void OnBeginFrame();
	void OnEndFrame() const;

	ImGuiContext* Context = nullptr;
	ImPlotContext* PlotContext = nullptr;

	char IniFilenameAnsi[1024] = {};
	char LogFilenameAnsi[1024] = {};

	TStrongObjectPtr<UTexture2D> FontAtlasTexturePtr = nullptr;
};
