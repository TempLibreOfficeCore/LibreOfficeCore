/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * This file incorporates work covered by the following license notice:
 *
 *   Licensed to the Apache Software Foundation (ASF) under one or more
 *   contributor license agreements. See the NOTICE file distributed
 *   with this work for additional information regarding copyright
 *   ownership. The ASF licenses this file to you under the Apache
 *   License, Version 2.0 (the "License"); you may not use this file
 *   except in compliance with the License. You may obtain a copy of
 *   the License at http://www.apache.org/licenses/LICENSE-2.0 .
 */

#include <vcl/svapp.hxx>
#include <vcl/settings.hxx>
#include <sal/log.hxx>
#include <tools/debug.hxx>
#include <comphelper/diagnose_ex.hxx>
#include <svtools/valueset.hxx>
#include "valueimp.hxx"
#include <comphelper/servicehelper.hxx>
#include <com/sun/star/accessibility/AccessibleEventId.hpp>
#include <com/sun/star/accessibility/AccessibleRole.hpp>
#include <com/sun/star/accessibility/AccessibleStateType.hpp>
#include <com/sun/star/lang/IndexOutOfBoundsException.hpp>
#include <com/sun/star/uno/RuntimeException.hpp>

using namespace ::com::sun::star;


ValueSetItem::ValueSetItem( ValueSet& rParent )
    : mrParent(rParent)
    , mpData(nullptr)
    , mxAcc()
    , mnId(0)
    , meType(VALUESETITEM_NONE)
    , mbVisible(true)
{
}


ValueSetItem::~ValueSetItem()
{
    if( mxAcc.is() )
    {
        mxAcc->ParentDestroyed();
    }
}

rtl::Reference< ValueItemAcc > ValueSetItem::GetAccessible( bool bIsTransientChildrenDisabled )
{
    if( !mxAcc.is() )
        mxAcc = new ValueItemAcc( this, bIsTransientChildrenDisabled );

    return mxAcc;
}

ValueItemAcc::ValueItemAcc( ValueSetItem* pParent, bool bIsTransientChildrenDisabled ) :
    mpParent( pParent ),
    mbIsTransientChildrenDisabled( bIsTransientChildrenDisabled )
{
}

ValueItemAcc::~ValueItemAcc()
{
}

void ValueItemAcc::ParentDestroyed()
{
    std::scoped_lock aGuard( maMutex );
    mpParent = nullptr;
}

uno::Reference< accessibility::XAccessibleContext > SAL_CALL ValueItemAcc::getAccessibleContext()
{
    return this;
}


sal_Int64 SAL_CALL ValueItemAcc::getAccessibleChildCount()
{
    return 0;
}


uno::Reference< accessibility::XAccessible > SAL_CALL ValueItemAcc::getAccessibleChild( sal_Int64 )
{
    throw lang::IndexOutOfBoundsException();
}


uno::Reference< accessibility::XAccessible > SAL_CALL ValueItemAcc::getAccessibleParent()
{
    const SolarMutexGuard aSolarGuard;
    uno::Reference< accessibility::XAccessible >    xRet;

    if( mpParent )
        xRet = mpParent->mrParent.mxAccessible;

    return xRet;
}


sal_Int64 SAL_CALL ValueItemAcc::getAccessibleIndexInParent()
{
    const SolarMutexGuard aSolarGuard;
    // The index defaults to -1 to indicate the child does not belong to its
    // parent.
    sal_Int64 nIndexInParent = -1;

    if( mpParent )
    {
        bool bDone = false;

        sal_uInt16 nCount = mpParent->mrParent.ImplGetVisibleItemCount();
        ValueSetItem* pItem;
        for (sal_uInt16 i=0; i<nCount && !bDone; i++)
        {
            // Guard the retrieval of the i-th child with a try/catch block
            // just in case the number of children changes in the meantime.
            try
            {
                pItem = mpParent->mrParent.ImplGetItem(i);
            }
            catch (const lang::IndexOutOfBoundsException&)
            {
                pItem = nullptr;
            }

            // Do not create an accessible object for the test.
            if (pItem != nullptr && pItem->mxAcc.is())
                if (pItem->GetAccessible( mbIsTransientChildrenDisabled ).get() == this )
                {
                    nIndexInParent = i;
                    bDone = true;
                }
        }
    }

    //if this valueset contain a none field(common value is default), then we should increase the real index and set the noitem index value equal 0.
    if ( mpParent && ( (mpParent->mrParent.GetStyle() & WB_NONEFIELD) != 0 ) )
    {
        ValueSetItem* pFirstItem = mpParent->mrParent.ImplGetItem (VALUESET_ITEM_NONEITEM);
        if( pFirstItem && pFirstItem ->GetAccessible(mbIsTransientChildrenDisabled).get() == this )
            nIndexInParent = 0;
        else
            nIndexInParent++;
    }
    return nIndexInParent;
}


sal_Int16 SAL_CALL ValueItemAcc::getAccessibleRole()
{
    return accessibility::AccessibleRole::LIST_ITEM;
}


OUString SAL_CALL ValueItemAcc::getAccessibleDescription()
{
    return OUString();
}


OUString SAL_CALL ValueItemAcc::getAccessibleName()
{
    const SolarMutexGuard aSolarGuard;

    if( mpParent )
    {
        if (mpParent->maText.isEmpty())
            return "Item " +  OUString::number(static_cast<sal_Int32>(mpParent->mnId));
        else
            return mpParent->maText;
    }

    return OUString();
}


uno::Reference< accessibility::XAccessibleRelationSet > SAL_CALL ValueItemAcc::getAccessibleRelationSet()
{
    return uno::Reference< accessibility::XAccessibleRelationSet >();
}


sal_Int64 SAL_CALL ValueItemAcc::getAccessibleStateSet()
{
    const SolarMutexGuard aSolarGuard;
    sal_Int64 nStateSet = 0;

    if( mpParent )
    {
        nStateSet |= accessibility::AccessibleStateType::ENABLED;
        nStateSet |= accessibility::AccessibleStateType::SENSITIVE;
        nStateSet |= accessibility::AccessibleStateType::SHOWING;
        nStateSet |= accessibility::AccessibleStateType::VISIBLE;
        if ( !mbIsTransientChildrenDisabled )
            nStateSet |= accessibility::AccessibleStateType::TRANSIENT;

        nStateSet |= accessibility::AccessibleStateType::SELECTABLE;
        nStateSet |= accessibility::AccessibleStateType::FOCUSABLE;

        if( mpParent->mrParent.GetSelectedItemId() == mpParent->mnId )
        {

            nStateSet |= accessibility::AccessibleStateType::SELECTED;
            if (mpParent->mrParent.HasChildFocus())
                nStateSet |= accessibility::AccessibleStateType::FOCUSED;
        }
    }

    return nStateSet;
}


lang::Locale SAL_CALL ValueItemAcc::getLocale()
{
    const SolarMutexGuard aSolarGuard;
    uno::Reference< accessibility::XAccessible >    xParent( getAccessibleParent() );
    lang::Locale                                    aRet( u""_ustr, u""_ustr, u""_ustr );

    if( xParent.is() )
    {
        uno::Reference< accessibility::XAccessibleContext > xParentContext( xParent->getAccessibleContext() );

        if( xParentContext.is() )
            aRet = xParentContext->getLocale();
    }

    return aRet;
}


void SAL_CALL ValueItemAcc::addAccessibleEventListener( const uno::Reference< accessibility::XAccessibleEventListener >& rxListener )
{
    std::scoped_lock aGuard( maMutex );

    if( !rxListener.is() )
           return;

    bool bFound = false;

    for (auto const& eventListener : mxEventListeners)
    {
        if(eventListener == rxListener)
        {
            bFound = true;
            break;
        }
    }

    if (!bFound)
        mxEventListeners.push_back( rxListener );
}


void SAL_CALL ValueItemAcc::removeAccessibleEventListener( const uno::Reference< accessibility::XAccessibleEventListener >& rxListener )
{
    std::scoped_lock aGuard( maMutex );

    if( rxListener.is() )
    {
        ::std::vector< uno::Reference< accessibility::XAccessibleEventListener > >::iterator aIter =
            std::find(mxEventListeners.begin(), mxEventListeners.end(), rxListener);

        if (aIter != mxEventListeners.end())
            mxEventListeners.erase(aIter);
    }
}


sal_Bool SAL_CALL ValueItemAcc::containsPoint( const awt::Point& aPoint )
{
    const awt::Rectangle    aRect( getBounds() );
    const Point             aSize( aRect.Width, aRect.Height );
    const Point             aNullPoint, aTestPoint( aPoint.X, aPoint.Y );

    return tools::Rectangle( aNullPoint, aSize ).Contains( aTestPoint );
}

uno::Reference< accessibility::XAccessible > SAL_CALL ValueItemAcc::getAccessibleAtPoint( const awt::Point& )
{
    uno::Reference< accessibility::XAccessible > xRet;
    return xRet;
}

awt::Rectangle SAL_CALL ValueItemAcc::getBounds()
{
    const SolarMutexGuard aSolarGuard;
    awt::Rectangle      aRet;

    if( mpParent )
    {
        tools::Rectangle   aRect( mpParent->mrParent.GetItemRect(mpParent->mnId) );
        tools::Rectangle   aParentRect( Point(), mpParent->mrParent.GetOutputSizePixel() );

        aRect.Intersection( aParentRect );

        aRet.X = aRect.Left();
        aRet.Y = aRect.Top();
        aRet.Width = aRect.GetWidth();
        aRet.Height = aRect.GetHeight();
    }

    return aRet;
}

awt::Point SAL_CALL ValueItemAcc::getLocation()
{
    const awt::Rectangle    aRect( getBounds() );
    awt::Point              aRet;

    aRet.X = aRect.X;
    aRet.Y = aRect.Y;

    return aRet;
}

awt::Point SAL_CALL ValueItemAcc::getLocationOnScreen()
{
    const SolarMutexGuard aSolarGuard;
    awt::Point          aRet;

    if( mpParent )
    {
        const Point aPos = mpParent->mrParent.GetItemRect(mpParent->mnId).TopLeft();
        const Point aScreenPos(mpParent->mrParent.GetDrawingArea()->get_accessible_location_on_screen());

        aRet.X = aPos.X() + aScreenPos.X();
        aRet.Y = aPos.Y() + aScreenPos.Y();
    }

    return aRet;
}

awt::Size SAL_CALL ValueItemAcc::getSize()
{
    const awt::Rectangle    aRect( getBounds() );
    awt::Size               aRet;

    aRet.Width = aRect.Width;
    aRet.Height = aRect.Height;

    return aRet;
}

void SAL_CALL ValueItemAcc::grabFocus()
{
    // nothing to do
}

sal_Int32 SAL_CALL ValueItemAcc::getForeground(  )
{
    Color nColor = Application::GetSettings().GetStyleSettings().GetWindowTextColor();
    return static_cast<sal_Int32>(nColor);
}

sal_Int32 SAL_CALL ValueItemAcc::getBackground(  )
{
    Color nColor;
    if (mpParent && mpParent->meType == VALUESETITEM_COLOR)
        nColor = mpParent->maColor;
    else
        nColor = Application::GetSettings().GetStyleSettings().GetWindowColor();
    return static_cast<sal_Int32>(nColor);
}

void ValueItemAcc::FireAccessibleEvent( short nEventId, const uno::Any& rOldValue, const uno::Any& rNewValue )
{
    if( !nEventId )
        return;

    ::std::vector< uno::Reference< accessibility::XAccessibleEventListener > >                  aTmpListeners( mxEventListeners );
    accessibility::AccessibleEventObject                                                        aEvtObject;

    aEvtObject.EventId = nEventId;
    aEvtObject.Source = getXWeak();
    aEvtObject.NewValue = rNewValue;
    aEvtObject.OldValue = rOldValue;

    for (auto const& tmpListener : aTmpListeners)
    {
        tmpListener->notifyEvent( aEvtObject );
    }
}

ValueSetAcc::ValueSetAcc( ValueSet* pParent ) :
    mpParent( pParent ),
    mbIsFocused(false)
{
}


ValueSetAcc::~ValueSetAcc()
{
}


void ValueSetAcc::FireAccessibleEvent( short nEventId, const uno::Any& rOldValue, const uno::Any& rNewValue )
{
    if( !nEventId )
        return;

    ::std::vector< uno::Reference< accessibility::XAccessibleEventListener > >                  aTmpListeners( mxEventListeners );
    accessibility::AccessibleEventObject                                                        aEvtObject;

    aEvtObject.EventId = nEventId;
    aEvtObject.Source = getXWeak();
    aEvtObject.NewValue = rNewValue;
    aEvtObject.OldValue = rOldValue;
    aEvtObject.IndexHint = -1;

    for (auto const& tmpListener : aTmpListeners)
    {
        try
        {
            tmpListener->notifyEvent( aEvtObject );
        }
        catch(const uno::Exception&)
        {
        }
    }
}

void ValueSetAcc::GetFocus()
{
    mbIsFocused = true;

    // Broadcast the state change.
    css::uno::Any aOldState, aNewState;
    aNewState <<= css::accessibility::AccessibleStateType::FOCUSED;
    FireAccessibleEvent(
        css::accessibility::AccessibleEventId::STATE_CHANGED,
        aOldState, aNewState);
}


void ValueSetAcc::LoseFocus()
{
    mbIsFocused = false;

    // Broadcast the state change.
    css::uno::Any aOldState, aNewState;
    aOldState <<= css::accessibility::AccessibleStateType::FOCUSED;
    FireAccessibleEvent(
        css::accessibility::AccessibleEventId::STATE_CHANGED,
        aOldState, aNewState);
}


uno::Reference< accessibility::XAccessibleContext > SAL_CALL ValueSetAcc::getAccessibleContext()
{
    // still allow retrieving a11y context when not disposed yet, but ValueSet is unset
    ThrowIfDisposed(false);
    return this;
}


sal_Int64 SAL_CALL ValueSetAcc::getAccessibleChildCount()
{
    const SolarMutexGuard aSolarGuard;
    ThrowIfDisposed();

    sal_Int64 nCount = mpParent->ImplGetVisibleItemCount();
    if (HasNoneField())
        nCount += 1;
    return nCount;
}


uno::Reference< accessibility::XAccessible > SAL_CALL ValueSetAcc::getAccessibleChild( sal_Int64 i )
{
    ThrowIfDisposed();
    const SolarMutexGuard aSolarGuard;

    if (i < 0 || i >= getAccessibleChildCount())
        throw lang::IndexOutOfBoundsException();

    ValueSetItem* pItem = getItem (sal::static_int_cast< sal_uInt16 >(i));

    if( !pItem )
        throw lang::IndexOutOfBoundsException();

    uno::Reference< accessibility::XAccessible >  xRet = pItem->GetAccessible( false/*bIsTransientChildrenDisabled*/ );
    return xRet;
}

uno::Reference< accessibility::XAccessible > SAL_CALL ValueSetAcc::getAccessibleParent()
{
    ThrowIfDisposed();
    const SolarMutexGuard aSolarGuard;
    return mpParent->GetDrawingArea()->get_accessible_parent();
}

sal_Int64 SAL_CALL ValueSetAcc::getAccessibleIndexInParent()
{
    ThrowIfDisposed();
    const SolarMutexGuard aSolarGuard;

    // -1 for child not found/no parent (according to specification)
    sal_Int64 nRet = -1;

    uno::Reference<accessibility::XAccessible> xParent(getAccessibleParent());
    if (!xParent)
        return nRet;

    try
    {
        uno::Reference<accessibility::XAccessibleContext> xParentContext(xParent->getAccessibleContext());

        //  iterate over parent's children and search for this object
        if ( xParentContext.is() )
        {
            sal_Int64 nChildCount = xParentContext->getAccessibleChildCount();
            for ( sal_Int64 nChild = 0; ( nChild < nChildCount ) && ( -1 == nRet ); ++nChild )
            {
                uno::Reference<XAccessible> xChild(xParentContext->getAccessibleChild(nChild));
                if ( xChild.get() == this )
                    nRet = nChild;
            }
        }
    }
    catch (const uno::Exception&)
    {
        TOOLS_WARN_EXCEPTION( "svtools", "ValueSetAcc::getAccessibleIndexInParent" );
    }

    return nRet;
}

sal_Int16 SAL_CALL ValueSetAcc::getAccessibleRole()
{
    ThrowIfDisposed();
    return accessibility::AccessibleRole::LIST;
}


OUString SAL_CALL ValueSetAcc::getAccessibleDescription()
{
    ThrowIfDisposed();
    const SolarMutexGuard aSolarGuard;
    OUString              aRet;

    if (mpParent)
    {
        aRet = mpParent->GetAccessibleDescription();
    }

    return aRet;
}


OUString SAL_CALL ValueSetAcc::getAccessibleName()
{
    ThrowIfDisposed();
    const SolarMutexGuard aSolarGuard;
    OUString              aRet;

    if (mpParent)
    {
        aRet = mpParent->GetAccessibleName();
    }

    return aRet;
}

uno::Reference< accessibility::XAccessibleRelationSet > SAL_CALL ValueSetAcc::getAccessibleRelationSet()
{
    ThrowIfDisposed();
    SolarMutexGuard g;
    return mpParent->GetDrawingArea()->get_accessible_relation_set();
}

sal_Int64 SAL_CALL ValueSetAcc::getAccessibleStateSet()
{
    ThrowIfDisposed();
    sal_Int64 nStateSet = 0;

    // Set some states.
    nStateSet |= accessibility::AccessibleStateType::ENABLED;
    nStateSet |= accessibility::AccessibleStateType::SENSITIVE;
    nStateSet |= accessibility::AccessibleStateType::SHOWING;
    nStateSet |= accessibility::AccessibleStateType::VISIBLE;
    nStateSet |= accessibility::AccessibleStateType::MANAGES_DESCENDANTS;
    nStateSet |= accessibility::AccessibleStateType::FOCUSABLE;
    if (mbIsFocused)
        nStateSet |= accessibility::AccessibleStateType::FOCUSED;

    return nStateSet;
}


lang::Locale SAL_CALL ValueSetAcc::getLocale()
{
    ThrowIfDisposed();
    const SolarMutexGuard aSolarGuard;
    uno::Reference< accessibility::XAccessible >    xParent( getAccessibleParent() );
    lang::Locale                                    aRet( u""_ustr, u""_ustr, u""_ustr );

    if( xParent.is() )
    {
        uno::Reference< accessibility::XAccessibleContext > xParentContext( xParent->getAccessibleContext() );

        if( xParentContext.is() )
            aRet = xParentContext->getLocale ();
    }

    return aRet;
}


void SAL_CALL ValueSetAcc::addAccessibleEventListener( const uno::Reference< accessibility::XAccessibleEventListener >& rxListener )
{
    ThrowIfDisposed(false);
    std::unique_lock aGuard (m_aMutex);

    if( !rxListener.is() )
           return;

    bool bFound = false;

    for (auto const& eventListener : mxEventListeners)
    {
        if(eventListener == rxListener)
        {
            bFound = true;
            break;
        }
    }

    if (!bFound)
        mxEventListeners.push_back( rxListener );
}


void SAL_CALL ValueSetAcc::removeAccessibleEventListener( const uno::Reference< accessibility::XAccessibleEventListener >& rxListener )
{
    ThrowIfDisposed(false);
    std::unique_lock aGuard (m_aMutex);

    if( rxListener.is() )
    {
        ::std::vector< uno::Reference< accessibility::XAccessibleEventListener > >::iterator aIter =
            std::find(mxEventListeners.begin(), mxEventListeners.end(), rxListener);

        if (aIter != mxEventListeners.end())
            mxEventListeners.erase(aIter);
    }
}


sal_Bool SAL_CALL ValueSetAcc::containsPoint( const awt::Point& aPoint )
{
    ThrowIfDisposed();
    const awt::Rectangle    aRect( getBounds() );
    const Point             aSize( aRect.Width, aRect.Height );
    const Point             aNullPoint, aTestPoint( aPoint.X, aPoint.Y );

    return tools::Rectangle( aNullPoint, aSize ).Contains( aTestPoint );
}


uno::Reference< accessibility::XAccessible > SAL_CALL ValueSetAcc::getAccessibleAtPoint( const awt::Point& aPoint )
{
    ThrowIfDisposed();
    const SolarMutexGuard aSolarGuard;
    const sal_uInt16                                    nItemId = mpParent->GetItemId( Point( aPoint.X, aPoint.Y ) );
    uno::Reference< accessibility::XAccessible >    xRet;

    if ( nItemId )
    {
        const size_t nItemPos = mpParent->GetItemPos( nItemId );

        if( VALUESET_ITEM_NONEITEM != nItemPos )
        {
            ValueSetItem *const pItem = mpParent->mItemList[nItemPos].get();
            xRet = pItem->GetAccessible( false/*bIsTransientChildrenDisabled*/ );
        }
    }

    return xRet;
}


awt::Rectangle SAL_CALL ValueSetAcc::getBounds()
{
    ThrowIfDisposed();
    const SolarMutexGuard aSolarGuard;
    const Point         aOutPos;
    const Size          aOutSize( mpParent->GetOutputSizePixel() );
    awt::Rectangle      aRet;

    aRet.X = aOutPos.X();
    aRet.Y = aOutPos.Y();
    aRet.Width = aOutSize.Width();
    aRet.Height = aOutSize.Height();

    return aRet;
}

awt::Point SAL_CALL ValueSetAcc::getLocation()
{
    ThrowIfDisposed();
    const awt::Rectangle    aRect( getBounds() );
    awt::Point              aRet;

    aRet.X = aRect.X;
    aRet.Y = aRect.Y;

    return aRet;
}

awt::Point SAL_CALL ValueSetAcc::getLocationOnScreen()
{
    ThrowIfDisposed();
    const SolarMutexGuard aSolarGuard;
    awt::Point aScreenLoc(0, 0);

    uno::Reference<accessibility::XAccessible> xParent(getAccessibleParent());
    if (xParent)
    {
        uno::Reference<accessibility::XAccessibleContext> xParentContext(xParent->getAccessibleContext());
        uno::Reference<accessibility::XAccessibleComponent> xParentComponent(xParentContext, css::uno::UNO_QUERY);
        OSL_ENSURE( xParentComponent.is(), "ValueSetAcc::getLocationOnScreen: no parent component!" );
        if ( xParentComponent.is() )
        {
            awt::Point aParentScreenLoc( xParentComponent->getLocationOnScreen() );
            awt::Point aOwnRelativeLoc( getLocation() );
            aScreenLoc.X = aParentScreenLoc.X + aOwnRelativeLoc.X;
            aScreenLoc.Y = aParentScreenLoc.Y + aOwnRelativeLoc.Y;
        }
    }

    return aScreenLoc;
}

awt::Size SAL_CALL ValueSetAcc::getSize()
{
    ThrowIfDisposed();
    const awt::Rectangle    aRect( getBounds() );
    awt::Size               aRet;

    aRet.Width = aRect.Width;
    aRet.Height = aRect.Height;

    return aRet;
}

void SAL_CALL ValueSetAcc::grabFocus()
{
    ThrowIfDisposed();
    const SolarMutexGuard aSolarGuard;
    mpParent->GrabFocus();
}

sal_Int32 SAL_CALL ValueSetAcc::getForeground(  )
{
    ThrowIfDisposed();
    Color nColor = Application::GetSettings().GetStyleSettings().GetWindowTextColor();
    return static_cast<sal_Int32>(nColor);
}

sal_Int32 SAL_CALL ValueSetAcc::getBackground(  )
{
    ThrowIfDisposed();
    Color nColor = Application::GetSettings().GetStyleSettings().GetWindowColor();
    return static_cast<sal_Int32>(nColor);
}

void SAL_CALL ValueSetAcc::selectAccessibleChild( sal_Int64 nChildIndex )
{
    ThrowIfDisposed();
    const SolarMutexGuard aSolarGuard;

    if (nChildIndex < 0 || nChildIndex >= getAccessibleChildCount())
        throw lang::IndexOutOfBoundsException();

    ValueSetItem* pItem = getItem (sal::static_int_cast< sal_uInt16 >(nChildIndex));

    if(pItem == nullptr)
        throw lang::IndexOutOfBoundsException();

    mpParent->SelectItem( pItem->mnId );
}


sal_Bool SAL_CALL ValueSetAcc::isAccessibleChildSelected( sal_Int64 nChildIndex )
{
    ThrowIfDisposed();
    const SolarMutexGuard aSolarGuard;

    if (nChildIndex < 0 || nChildIndex >= getAccessibleChildCount())
        throw lang::IndexOutOfBoundsException();

    ValueSetItem* pItem = getItem (sal::static_int_cast< sal_uInt16 >(nChildIndex));

    if (pItem == nullptr)
        throw lang::IndexOutOfBoundsException();

    bool  bRet = mpParent->IsItemSelected( pItem->mnId );
    return bRet;
}


void SAL_CALL ValueSetAcc::clearAccessibleSelection()
{
    ThrowIfDisposed();
    const SolarMutexGuard aSolarGuard;
    mpParent->SetNoSelection();
}


void SAL_CALL ValueSetAcc::selectAllAccessibleChildren()
{
    ThrowIfDisposed();
    // unsupported due to single selection only
}


sal_Int64 SAL_CALL ValueSetAcc::getSelectedAccessibleChildCount()
{
    ThrowIfDisposed();
    const SolarMutexGuard aSolarGuard;
    sal_Int64 nRet = 0;

    for( sal_uInt16 i = 0, nCount = getItemCount(); i < nCount; i++ )
    {
        ValueSetItem* pItem = getItem (i);

        if( pItem && mpParent->IsItemSelected( pItem->mnId ) )
            ++nRet;
    }

    return nRet;
}


uno::Reference< accessibility::XAccessible > SAL_CALL ValueSetAcc::getSelectedAccessibleChild( sal_Int64 nSelectedChildIndex )
{
    ThrowIfDisposed();
    const SolarMutexGuard aSolarGuard;
    uno::Reference< accessibility::XAccessible >    xRet;

    for( sal_uInt16 i = 0, nCount = getItemCount(), nSel = 0; ( i < nCount ) && !xRet.is(); i++ )
    {
        ValueSetItem* pItem = getItem(i);

        if( pItem && mpParent->IsItemSelected( pItem->mnId ) && ( nSelectedChildIndex == static_cast< sal_Int64 >( nSel++ ) ) )
            xRet = pItem->GetAccessible( false/*bIsTransientChildrenDisabled*/ );
    }

    return xRet;
}


void SAL_CALL ValueSetAcc::deselectAccessibleChild( sal_Int64 nChildIndex )
{
    ThrowIfDisposed();
    const SolarMutexGuard aSolarGuard;

    if (nChildIndex < 0 || nChildIndex >= getAccessibleChildCount())
        throw lang::IndexOutOfBoundsException();

    // Because of the single selection we can reset the whole selection when
    // the specified child is currently selected.
    if (isAccessibleChildSelected(nChildIndex))
        mpParent->SetNoSelection();
}

void ValueSetAcc::Invalidate()
{
    mpParent = nullptr;
}

void ValueSetAcc::disposing(std::unique_lock<std::mutex>& rGuard)
{
    // Make a copy of the list and clear the original.
    ::std::vector<uno::Reference<accessibility::XAccessibleEventListener> > aListenerListCopy = std::move(mxEventListeners);

    if (aListenerListCopy.empty())
        return;

    rGuard.unlock();
    // Inform all listeners that this objects is disposing.
    lang::EventObject aEvent (static_cast<accessibility::XAccessible*>(this));
    for (auto const& listenerCopy : aListenerListCopy)
    {
        try
        {
            listenerCopy->disposing (aEvent);
        }
        catch(const uno::Exception&)
        {
            // Ignore exceptions.
        }
    }
}


sal_uInt16 ValueSetAcc::getItemCount() const
{
    sal_uInt16 nCount = mpParent->ImplGetVisibleItemCount();
    // When the None-Item is visible then increase the number of items by
    // one.
    if (HasNoneField())
        nCount += 1;
    return nCount;
}

ValueSetItem* ValueSetAcc::getItem (sal_uInt16 nIndex) const
{
    ValueSetItem* pItem = nullptr;

    if (HasNoneField())
    {
        if (nIndex == 0)
            // When present the first item is the then always visible none field.
            pItem = mpParent->ImplGetItem (VALUESET_ITEM_NONEITEM);
        else
            // Shift down the index to compensate for the none field.
            nIndex -= 1;
    }
    if (pItem == nullptr)
        pItem = mpParent->ImplGetItem (nIndex);

    return pItem;
}


void ValueSetAcc::ThrowIfDisposed(bool bCheckParent)
{
    if (m_bDisposed)
    {
        SAL_WARN("svx", "Calling disposed object. Throwing exception:");
        throw lang::DisposedException (
            u"object has been already disposed"_ustr,
            getXWeak());
    }

    if (bCheckParent && !mpParent)
    {
        assert(false && "ValueSetAcc not disposed but mpParent == NULL");
        throw css::uno::RuntimeException(u"ValueSetAcc not disposed but mpParent == NULL"_ustr);
    }
}

bool ValueSetAcc::HasNoneField() const
{
    assert(mpParent && "ValueSetAcc::HasNoneField called with mpParent==NULL");
    return ((mpParent->GetStyle() & WB_NONEFIELD) != 0);
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
