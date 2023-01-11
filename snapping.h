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

    enum Modes : unsigned
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
    ~Snapping() = default;
    
	std::optional<CPoint> Get(const CDrawObj* pSelectedObj, const CPoint& ptStart, float dist);

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