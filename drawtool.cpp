// drawtool.cpp - implementation for drawing tools
//
// This is a part of the Microsoft Foundation Classes C++ library.
// Copyright (c) Microsoft Corporation.  All rights reserved.
//
// This source code is only intended as a supplement to the
// Microsoft Foundation Classes Reference and related
// electronic documentation provided with the library.
// See these sources for detailed information regarding the
// Microsoft Foundation Classes product.


#include "stdafx.h"
#include "drawcli.h"

#include "drawdoc.h"
#include "drawvw.h"
#include "drawobj.h"
#include "drawtool.h"

#include <optional>

/////////////////////////////////////////////////////////////////////////////
// CDrawTool implementation

CPtrList CDrawTool::c_tools;

static CSelectTool selectTool;
static CRectTool lineTool(line);
static CRectTool rectTool(rect);
static CRectTool roundRectTool(roundRect);
static CRectTool ellipseTool(ellipse);
static CPolyTool polyTool;

/////////////////////////////////////////////////////////////////////////////
// CDrawTool

CPoint CDrawTool::c_down;
UINT CDrawTool::c_nDownFlags;
CPoint CDrawTool::c_last;
DrawShape CDrawTool::c_drawShape = selection;

CDrawTool::CDrawTool(DrawShape drawShape)
{
	m_drawShape = drawShape;
	c_tools.AddTail(this);
}

CDrawTool* CDrawTool::FindTool(DrawShape drawShape)
{
	POSITION pos = c_tools.GetHeadPosition();
	while (pos != NULL)
	{
		CDrawTool* pTool = (CDrawTool*)c_tools.GetNext(pos);
		if (pTool->m_drawShape == drawShape)
			return pTool;
	}

	return NULL;
}

void CDrawTool::OnLButtonDown(CDrawView* pView, UINT nFlags,
	const CPoint& point)
{
	// deactivate any in-place active item on this view!
	COleClientItem* pActiveItem = pView->GetDocument()->GetInPlaceActiveItem(
		pView);
	if (pActiveItem != NULL)
	{
		pActiveItem->Close();
		ASSERT(pView->GetDocument()->GetInPlaceActiveItem(pView) == NULL);
	}

	pView->SetCapture();
	c_nDownFlags = nFlags;
	c_down = point;
	c_last = point;
}

void CDrawTool::OnLButtonDblClk(CDrawView* /*pView*/, UINT /*nFlags*/,
	const CPoint& /*point*/)
{
}

void CDrawTool::OnLButtonUp(CDrawView* /*pView*/, UINT /*nFlags*/,
	const CPoint& point)
{
	ReleaseCapture();

	if (point == c_down)
		c_drawShape = selection;
}

void CDrawTool::OnMouseMove(CDrawView* /*pView*/, UINT /*nFlags*/,
	const CPoint& point)
{
	c_last = point;
	SetCursor(AfxGetApp()->LoadStandardCursor(IDC_ARROW));
}

void CDrawTool::OnEditProperties(CDrawView* /*pView*/)
{
}

void CDrawTool::OnCancel()
{
	c_drawShape = selection;
}

////////////////////////////////////////////////////////////////////////////
// CResizeTool

enum SelectMode
{
	none,
	netSelect,
	move,
	size
};

SelectMode selectMode = none;
int nDragHandle;

CPoint ptLast;

CSelectTool::CSelectTool() :
	CDrawTool(selection)
{
}

void CSelectTool::OnLButtonDown(CDrawView* pView, UINT nFlags,
	const CPoint& point)
{
	CPoint ptLocal = point;
	pView->ClientToDoc(ptLocal);

	CDrawObj* pObj;
	selectMode = none;

	// Check for resizing (only allowed on single selections)
	if (pView->m_selection.GetCount() == 1)
	{
		pObj = pView->m_selection.GetHead();
		nDragHandle = pObj->HitTest(ptLocal, pView, TRUE);
		if (nDragHandle != 0)
			selectMode = size;
	}

	// See if the click was on an object, select and start move if so
	if (selectMode == none)
	{
		pObj = pView->GetDocument()->ObjectAt(ptLocal);

		if (pObj != NULL)
		{
			selectMode = move;

			if (!pView->IsSelected(pObj))
				pView->Select(pObj, (nFlags & MK_SHIFT) != 0);

			// Ctrl+Click clones the selection...
			if ((nFlags & MK_CONTROL) != 0)
				pView->CloneSelection();
		}
	}

	// Click on background, start a net-selection
	if (selectMode == none)
	{
		if ((nFlags & MK_SHIFT) == 0)
			pView->Select(NULL);

		selectMode = netSelect;

		CClientDC dc(pView);
		CRect rect(point.x, point.y, point.x, point.y);
		rect.NormalizeRect();
		dc.DrawFocusRect(rect);
	}

	ptLast = ptLocal;
	CDrawTool::OnLButtonDown(pView, nFlags, point);
}

void CSelectTool::OnLButtonDblClk(CDrawView* pView, UINT nFlags,
	const CPoint& point)
{
	if ((nFlags & MK_SHIFT) != 0)
	{
		// Shift+DblClk deselects object...
		CPoint ptLocal = point;
		pView->ClientToDoc(ptLocal);
		CDrawObj* pObj = pView->GetDocument()->ObjectAt(ptLocal);
		if (pObj != NULL)
			pView->Deselect(pObj);
	}
	else
	{
		// "Normal" DblClk opens properties, or OLE server...
		if (pView->m_selection.GetCount() == 1)
			pView->m_selection.GetHead()->OnOpen(pView);
	}

	CDrawTool::OnLButtonDblClk(pView, nFlags, point);
}

void CSelectTool::OnEditProperties(CDrawView* pView)
{
	if (pView->m_selection.GetCount() == 1)
		pView->m_selection.GetHead()->OnEditProperties();
}

void CSelectTool::OnLButtonUp(CDrawView* pView, UINT nFlags,
	const CPoint& point)
{
	if (pView->GetCapture() == pView)
	{
		if (selectMode == netSelect)
		{
			CClientDC dc(pView);
			CRect rect(c_down.x, c_down.y, c_last.x, c_last.y);
			rect.NormalizeRect();
			dc.DrawFocusRect(rect);

			pView->SelectWithinRect(rect, TRUE);
		}
		else if (selectMode != none)
		{
			pView->GetDocument()->UpdateAllViews(pView);
		}
	}

	CDrawTool::OnLButtonUp(pView, nFlags, point);
}

void CSelectTool::OnMouseMove(CDrawView* pView, UINT nFlags,
	const CPoint& point)
{
	if (pView->GetCapture() != pView)
	{
		if (c_drawShape == selection && pView->m_selection.GetCount() == 1)
		{
			CDrawObj* pObj = pView->m_selection.GetHead();
			CPoint ptLocal = point;
			pView->ClientToDoc(ptLocal);
			int nHandle = pObj->HitTest(ptLocal, pView, TRUE);
			if (nHandle != 0)
			{
				SetCursor(pObj->GetHandleCursor(nHandle));
				
				return; // bypass CDrawTool
			}
		}

		if (c_drawShape == selection)
			CDrawTool::OnMouseMove(pView, nFlags, point);
		
		return;
	}

	if (selectMode == netSelect)
	{
		CClientDC dc(pView);
		CRect rect(c_down.x, c_down.y, c_last.x, c_last.y);
		rect.NormalizeRect();
		dc.DrawFocusRect(rect);
		rect.SetRect(c_down.x, c_down.y, point.x, point.y);
		rect.NormalizeRect();
		dc.DrawFocusRect(rect);

		CDrawTool::OnMouseMove(pView, nFlags, point);
		
		return;
	}

	CPoint ptLocal = point;
	pView->ClientToDoc(ptLocal);
	CPoint delta = (CPoint)(ptLocal - ptLast);

	std::optional<CPoint> snap{ std::nullopt };
	POSITION pos = pView->m_selection.GetHeadPosition();
	while (pos != NULL)
	{
		CDrawObj* pObj = pView->m_selection.GetNext(pos);
		CRect position = pObj->m_rectPos;

		if (selectMode == move)
		{
			position += delta;
			pObj->MoveTo(position, pView);
		}
		else if (nDragHandle != 0)
		{
			// TODO: Isn't it suboptimal to call GetSnap for every MouseMove?
			//		 For dense views, this could be mitigated by using any of the well-known collision detection algorithms.
			//		 E.g. a quadtree or a spatial hash like this: https://ronandoherty.com/blog/spatial-hashing.
			snap = pView->GetSnap(pObj, ptLocal, 10);
			pObj->MoveHandleTo(nDragHandle, snap ? *snap : ptLocal, pView);
		}
	}

	ptLast = ptLocal;

	if (selectMode == size && c_drawShape == selection)
	{
		c_last = point;
		
		if (snap)
		{
			// Change the cursor to a hand-drawn one, to indicate that we are snapping to an endpoint
			// TODO: For multiple snapping operations at the same time, the cursor handling gets complex.
			//	     We should probably pre-generate the possible snap-cursor combinations before compilation
			//	     using a set of enum flags from Snapping class or similar.  
			//	     Otherwise we could consider an image dragging operation, to drag a compound image
			//	     indicator - generated on the fly - with the original cursor. This seems beyond the scope
			//	     of this sample task though.
			HCURSOR hCursor = LoadCursor(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDC_SNAP_ENDPOINT));
			SetCursor(hCursor);	
		}
		else
		{
			SetCursor(pView->m_selection.GetHead()->GetHandleCursor(nDragHandle));
		}
		return; // bypass CDrawTool
	}

	c_last = point;

	if (c_drawShape == selection)
		CDrawTool::OnMouseMove(pView, nFlags, point);
}

////////////////////////////////////////////////////////////////////////////
// CRectTool (does rectangles, round-rectangles, and ellipses)

CRectTool::CRectTool(DrawShape drawShape) :
	CDrawTool(drawShape)
{
}

void CRectTool::OnLButtonDown(CDrawView* pView, UINT nFlags,
	const CPoint& point)
{
	CDrawTool::OnLButtonDown(pView, nFlags, point);

	CPoint ptLocal = point;
	pView->ClientToDoc(ptLocal);

	CDrawRect* pObj = new CDrawRect(CRect(ptLocal, CSize(0, 0)));
	switch (m_drawShape)
	{
	case rect:
		pObj->m_nShape = CDrawRect::rectangle;
		break;

	case roundRect:
		pObj->m_nShape = CDrawRect::roundRectangle;
		break;

	case ellipse:
		pObj->m_nShape = CDrawRect::ellipse;
		break;

	case line:
		pObj->m_nShape = CDrawRect::line;
		break;

	default:
		ASSERT(FALSE); // unsuported shape!
	}

	pView->GetDocument()->Add(pObj);
	pView->Select(pObj);

	selectMode = size;
	nDragHandle = 1;
	ptLast = ptLocal;
}

void CRectTool::OnLButtonDblClk(CDrawView* pView, UINT nFlags,
	const CPoint& point)
{
	CDrawTool::OnLButtonDblClk(pView, nFlags, point);
}

void CRectTool::OnLButtonUp(CDrawView* pView, UINT nFlags,
	const CPoint& point)
{
	if (point == c_down)
	{
		// Don't create empty objects...
		CDrawObj* pObj = pView->m_selection.GetTail();
		pView->GetDocument()->Remove(pObj);
		pObj->Remove();
		selectTool.OnLButtonDown(pView, nFlags, point); // try a select!
	}

	selectTool.OnLButtonUp(pView, nFlags, point);
}

void CRectTool::OnMouseMove(CDrawView* pView, UINT nFlags,
	const CPoint& point)
{
	SetCursor(AfxGetApp()->LoadStandardCursor(IDC_CROSS));
	selectTool.OnMouseMove(pView, nFlags, point);
}


////////////////////////////////////////////////////////////////////////////
// CPolyTool

CPolyTool::CPolyTool() :
	CDrawTool(poly)
{
	m_pDrawObj = NULL;
}

void CPolyTool::OnLButtonDown(CDrawView* pView, UINT nFlags,
	const CPoint& point)
{
	CDrawTool::OnLButtonDown(pView, nFlags, point);

	CPoint ptLocal = point;
	pView->ClientToDoc(ptLocal);

	if (m_pDrawObj == NULL)
	{
		pView->SetCapture();

		m_pDrawObj = new CDrawPoly(CRect(ptLocal, CSize(0, 0)));
		pView->GetDocument()->Add(m_pDrawObj);
		pView->Select(m_pDrawObj);
		m_pDrawObj->AddPoint(ptLocal, pView);
	}
	else if (ptLocal == m_pDrawObj->m_points[0])
	{
		// Stop when the first point is repeated...
		ReleaseCapture();
		m_pDrawObj->m_nPoints -= 1;
		if (m_pDrawObj->m_nPoints < 2)
			m_pDrawObj->Remove();
		else
			pView->InvalObj(m_pDrawObj);
		m_pDrawObj = NULL;
		c_drawShape = selection;
		
		return;
	}

	ptLocal.x += 1; // adjacent points can't be the same!
	m_pDrawObj->AddPoint(ptLocal, pView);

	selectMode = size;
	nDragHandle = m_pDrawObj->GetHandleCount();
	ptLast = ptLocal;
}

void CPolyTool::OnLButtonUp(CDrawView* /*pView*/, UINT /*nFlags*/,
	const CPoint& /*point*/)
{
	// Don't release capture yet!
}

void CPolyTool::OnMouseMove(CDrawView* pView, UINT nFlags,
	const CPoint& point)
{
	if (m_pDrawObj != NULL && (nFlags & MK_LBUTTON) != 0)
	{
		CPoint ptLocal = point;
		pView->ClientToDoc(ptLocal);
		m_pDrawObj->AddPoint(ptLocal);
		nDragHandle = m_pDrawObj->GetHandleCount();
		ptLast = ptLocal;
		c_last = point;
		SetCursor(AfxGetApp()->LoadCursor(IDC_PENCIL));
	}
	else
	{
		SetCursor(AfxGetApp()->LoadStandardCursor(IDC_CROSS));
		selectTool.OnMouseMove(pView, nFlags, point);
	}
}

void CPolyTool::OnLButtonDblClk(CDrawView* pView, UINT , const CPoint& /*point*/)
{
	ReleaseCapture();

	int nPoints = m_pDrawObj->m_nPoints;
	if (nPoints > 2 &&
		(m_pDrawObj->m_points[nPoints - 1] == m_pDrawObj->m_points[nPoints - 2] ||
		m_pDrawObj->m_points[nPoints - 1].x - 1 == m_pDrawObj->m_points[nPoints - 2].x &&
		m_pDrawObj->m_points[nPoints - 1].y == m_pDrawObj->m_points[nPoints - 2].y))

	{
		// Nuke the last point if it's the same as the next to last...
		m_pDrawObj->m_nPoints -= 1;
		pView->InvalObj(m_pDrawObj);
	}

	m_pDrawObj = NULL;
	c_drawShape = selection;
}

void CPolyTool::OnCancel()
{
	CDrawTool::OnCancel();

	m_pDrawObj = NULL;
}

/////////////////////////////////////////////////////////////////////////////
