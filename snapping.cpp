#include "stdafx.h"
#include "snapping.h"
#include "drawdoc.h"

Snapping::Snapping(CDrawView& view)
    : m_view(view)
{
}

std::optional<CPoint> Snapping::Get(const CDrawObj* selectedObj, const CPoint& ptStart, float dist)
{
	auto pDoc = m_view.GetDocument();
	auto pObjs = pDoc->GetObjects();	
	auto dist2 = dist*dist;
	
	// Get handles==endpoints for all objects within distance and find nearest 
	auto pos = pObjs->GetHeadPosition();
	auto nearest = std::optional<CPoint>{std::nullopt};
	auto nearestDist2 = dist2;

	while (pos != NULL)
	{
		CDrawObj* pObj = pObjs->GetNext(pos);
		if (pObj == selectedObj)
			continue;

		const auto numHandles = pObj->GetHandleCount();
		for (int i=1; i<=numHandles; ++i)
		{
			CPoint ptHandle = pObj->GetHandle(i);

			// Exclude handles that are too far away using rectangle intersection 
			CRect rect(ptStart, ptStart);
			rect.InflateRect(dist, dist);
			if (!rect.PtInRect(ptHandle))
				continue;

			const float dx = ptStart.x - ptHandle.x;
			const float dy = ptStart.y - ptHandle.y;
			const float d2 = dx*dx + dy*dy;

			if (d2 <= nearestDist2)
			{
				nearest = ptHandle;
				nearestDist2 = d2;
			}
		}
	}

	return nearest;
}