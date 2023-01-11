#pragma once

#include <optional>

#include "drawvw.h"
#include "drawobj.h"

#include "stdafx.h"

/**
 * @brief Snapping info class
 */
class Snapping
{
public:

    enum Modes : char
    {
        SNAP_NONE          = 0x0,
        SNAP_ANY           = 0x1,
        SNAP_MID           = 0x2,
        SNAP_END           = 0x4,
        SNAP_CENTER        = 0x8,
        SNAP_TANGENT       = 0x10,
        SNAP_PERPENDICULAR = 0x20,
        SNAP_VERTICAL      = 0x40,
        SNAP_HORIZONTAL    = 0x80,
        SNAP_45            = 0x100,
    };

    Snapping(CDrawView& view);
    ~Snapping();
    
	std::optional<CPoint> GetSnapEndpoint(const CDrawObj* pSelectedObj, const CPoint& ptStart, float dist)
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
            if (pObj == pSelectedObj)
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

    /**
     * @brief Recalculate the snap points
     * 
     * @param pSnapPoint 
     */
    void Update(CDrawView* pView);

    /**
     * @brief Set the snap mode
     * 
     * @param snapMode 
     */
    void SetMode(int snapMode);
    /**
     * @brief Get the snap mode
     * 
     * @return int 
     */
    int GetMode() const;
    /**
     * @brief Set the snap distance
     * 
     * @param snapDistance 
     */
    void SetDist(float snapDistance);
    /**
     * @brief Get the snap distance
     * 
     * @return int 
     */
    float GetDist() const;
    
private:
    CDrawView& m_view; // The view to snap in to.
    // A list of snapping points.
    CTypedPtrList<CObList, CDrawObj*> m_snapPoints;
};