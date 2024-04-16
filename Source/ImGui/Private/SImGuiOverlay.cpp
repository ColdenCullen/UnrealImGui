#include "SImGuiOverlay.h"

#include <Framework/Application/SlateApplication.h>

#include "ImGuiContext.h"

FImGuiDrawList::FImGuiDrawList(ImDrawList* Source)
{
	VtxBuffer.swap(Source->VtxBuffer);
	IdxBuffer.swap(Source->IdxBuffer);
	CmdBuffer.swap(Source->CmdBuffer);
	Flags = Source->Flags;
}

FImGuiDrawData::FImGuiDrawData(const ImDrawData* Source)
{
	bValid = Source->Valid;

	TotalIdxCount = Source->TotalIdxCount;
	TotalVtxCount = Source->TotalVtxCount;

	DrawLists.SetNumUninitialized(Source->CmdListsCount);
	ConstructItems<FImGuiDrawList>(DrawLists.GetData(), Source->CmdLists.Data, Source->CmdListsCount);

	DisplayPos = Source->DisplayPos;
	DisplaySize = Source->DisplaySize;
	FrameBufferScale = Source->FramebufferScale;
}

void SImGuiOverlay::Construct(const FArguments& Args)
{
	SetVisibility(EVisibility::HitTestInvisible);

	Context = Args._Context;
	if (!Context.IsValid())
	{
		Context = FImGuiContext::Create();
	}
}

SImGuiOverlay::~SImGuiOverlay()
{
}

int32 SImGuiOverlay::OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	if (!DrawData.bValid)
	{
		return LayerId;
	}

	const FSlateRenderTransform Transform(AllottedGeometry.GetAccumulatedRenderTransform().GetTranslation() - FVector2d(DrawData.DisplayPos));

	FSlateBrush TextureBrush;
	for (const FImGuiDrawList& DrawList : DrawData.DrawLists)
	{
		TArray<FSlateVertex> Vertices;
		Vertices.SetNumUninitialized(DrawList.VtxBuffer.Size);
		for (int32 BufferIdx = 0; BufferIdx < Vertices.Num(); ++BufferIdx)
		{
			const ImDrawVert& Vtx = DrawList.VtxBuffer.Data[BufferIdx];
			Vertices[BufferIdx] = FSlateVertex::Make<ESlateVertexRounding::Disabled>(Transform, Vtx.pos, Vtx.uv, FVector2f::UnitVector, ImGui::ConvertColor(Vtx.col));
		}

		TArray<SlateIndex> Indices;
		Indices.SetNumUninitialized(DrawList.IdxBuffer.Size);
		for (int32 BufferIdx = 0; BufferIdx < Indices.Num(); ++BufferIdx)
		{
			Indices[BufferIdx] = DrawList.IdxBuffer.Data[BufferIdx];
		}

		for (const ImDrawCmd& DrawCmd : DrawList.CmdBuffer)
		{
			TArray VerticesSlice(Vertices.GetData() + DrawCmd.VtxOffset, Vertices.Num() - DrawCmd.VtxOffset);
			TArray IndicesSlice(Indices.GetData() + DrawCmd.IdxOffset, DrawCmd.ElemCount);

#if WITH_ENGINE
			UTexture2D* Texture = DrawCmd.GetTexID();
			if (TextureBrush.GetResourceObject() != Texture)
			{
				TextureBrush.SetResourceObject(Texture);
				if (IsValid(Texture))
				{
					TextureBrush.ImageSize.X = Texture->GetSizeX();
					TextureBrush.ImageSize.Y = Texture->GetSizeY();
					TextureBrush.ImageType = ESlateBrushImageType::FullColor;
					TextureBrush.DrawAs = ESlateBrushDrawType::Image;
				}
				else
				{
					TextureBrush.ImageSize.X = 0;
					TextureBrush.ImageSize.Y = 0;
					TextureBrush.ImageType = ESlateBrushImageType::NoImage;
					TextureBrush.DrawAs = ESlateBrushDrawType::NoDrawType;
				}
			}
#else
			FSlateBrush* Texture = DrawCmd.GetTexID();
			if (Texture)
			{
				TextureBrush = *Texture;
			}
			else
			{
				TextureBrush.ImageSize.X = 0;
				TextureBrush.ImageSize.Y = 0;
				TextureBrush.ImageType = ESlateBrushImageType::NoImage;
				TextureBrush.DrawAs = ESlateBrushDrawType::NoDrawType;
			}
#endif

			FSlateRect ClipRect(DrawCmd.ClipRect.x, DrawCmd.ClipRect.y, DrawCmd.ClipRect.z, DrawCmd.ClipRect.w);
			ClipRect = TransformRect(Transform, ClipRect);

			OutDrawElements.PushClip(FSlateClippingZone(ClipRect));
			FSlateDrawElement::MakeCustomVerts(OutDrawElements, LayerId, TextureBrush.GetRenderingResource(), VerticesSlice, IndicesSlice, nullptr, 0, 0);
			OutDrawElements.PopClip();
		}
	}

	return LayerId;
}

FVector2D SImGuiOverlay::ComputeDesiredSize(float LayoutScaleMultiplier) const
{
	return FVector2D::ZeroVector;
}

bool SImGuiOverlay::SupportsKeyboardFocus() const
{
	return true;
}

FReply SImGuiOverlay::OnKeyChar(const FGeometry& MyGeometry, const FCharacterEvent& Event)
{
	ImGui::FScopedContext ScopedContext(Context);

	ImGuiIO& IO = ImGui::GetIO();

	IO.AddInputCharacter(CharCast<ANSICHAR>(Event.GetCharacter()));

	return IO.WantCaptureKeyboard ? FReply::Handled() : FReply::Unhandled();
}

TSharedPtr<FImGuiContext> SImGuiOverlay::GetContext() const
{
	return Context;
}

void SImGuiOverlay::SetDrawData(const ImDrawData* InDrawData)
{
	DrawData = FImGuiDrawData(InDrawData);
}
