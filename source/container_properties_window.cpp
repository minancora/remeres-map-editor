//////////////////////////////////////////////////////////////////////
// This file is part of Remere's Map Editor
//////////////////////////////////////////////////////////////////////

#include "main.h"

#include "container_properties_window.h"

#include "old_properties_window.h"
#include "properties_window.h"
#include "gui.h"
#include "complexitem.h"
#include "map.h"

// ============================================================================
// Container Item Button
// Displayed in the container object properties menu, needs some
// custom event handling for the right-click menu etcetera so we
// need to define a custom class for it.

std::unique_ptr<ContainerItemPopupMenu> ContainerItemButton::popup_menu;

BEGIN_EVENT_TABLE(ContainerItemButton, ItemButton)
	EVT_LEFT_DOWN(ContainerItemButton::OnMouseDoubleLeftClick)
	EVT_RIGHT_UP(ContainerItemButton::OnMouseRightRelease)

	EVT_MENU(CONTAINER_POPUP_MENU_ADD, ContainerItemButton::OnAddItem)
	EVT_MENU(CONTAINER_POPUP_MENU_EDIT, ContainerItemButton::OnEditItem)
	EVT_MENU(CONTAINER_POPUP_MENU_REMOVE, ContainerItemButton::OnRemoveItem)
END_EVENT_TABLE()

ContainerItemButton::ContainerItemButton(wxWindow* parent, bool large, int _index, const Map* map, Item* item) :
	ItemButton(parent, (large? RENDER_SIZE_32x32 : RENDER_SIZE_16x16), (item? item->getClientID() : 0)),
	edit_map(map),
	edit_item(item),
	index(_index)
{
	////
}

ContainerItemButton::~ContainerItemButton()
{
	////
}

void ContainerItemButton::OnMouseDoubleLeftClick(wxMouseEvent& WXUNUSED(event))
{
	wxCommandEvent dummy;

	if(edit_item) {
		OnEditItem(dummy);
		return;
	}

	Container* container = getParentContainer();
	if(container->getVolume() > container->getItemCount()) {
		OnAddItem(dummy);
	}
}

void ContainerItemButton::OnMouseRightRelease(wxMouseEvent& WXUNUSED(event))
{
	if(!popup_menu) {
		popup_menu.reset(newd ContainerItemPopupMenu);
	}

	popup_menu->Update(this);
	PopupMenu(popup_menu.get());
}

void ContainerItemButton::OnAddItem(wxCommandEvent& WXUNUSED(event))
{
	FindItemDialog* itemDialog = newd FindItemDialog(GetParent(), wxT("Choose Item to add"));
	itemDialog->setCondition([](const ItemType& itemType) -> bool {
		return itemType.pickupable;
	});

	int32_t id = itemDialog->ShowModal();
	itemDialog->Destroy();

	if(id == 0) {
		return;
	}

	Container* container = getParentContainer();
	ItemVector& itemVector = container->getVector();

	Item* item = Item::Create(id);
	if(index < itemVector.size()) {
		itemVector.insert(itemVector.begin() + index, item);
	} else {
		itemVector.push_back(item);
	}

	ObjectPropertiesWindowBase* propertyWindow = getParentContainerWindow();
	if(propertyWindow) {
		propertyWindow->Update();
	}
}

void ContainerItemButton::OnEditItem(wxCommandEvent& WXUNUSED(event))
{
	ASSERT(edit_item);

	wxPoint newDialogAt;
	wxWindow* w = this;
	while((w = w->GetParent())) {
		if(ObjectPropertiesWindowBase* o = dynamic_cast<ObjectPropertiesWindowBase*>(w)) {
			newDialogAt = o->GetPosition();
			break;
		}
	}

	newDialogAt += wxPoint(20, 20);

	wxDialog* d;

	if(edit_map->getVersion().otbm >= MAP_OTBM_4)
		d = newd PropertiesWindow(this, edit_map, nullptr, edit_item, newDialogAt);
	else
		d = newd OldPropertiesWindow(this, edit_map, nullptr, edit_item, newDialogAt);

	d->ShowModal();
	d->Destroy();
}

void ContainerItemButton::OnRemoveItem(wxCommandEvent& WXUNUSED(event))
{
	ASSERT(edit_item);
	int32_t ret = gui.PopupDialog(GetParent(),
		wxT("Remove Item"),
		wxT("Are you sure you want to remove this item from the container?"),
		wxYES | wxNO
	);

	if(ret != wxID_YES) {
		return;
	}

	Container* container = getParentContainer();
	ItemVector& itemVector = container->getVector();

	auto it = itemVector.begin();
	for(; it != itemVector.end(); ++it) {
		if(*it == edit_item) {
			break;
		}
	}

	ASSERT(it != itemVector.end());

	itemVector.erase(it);
	delete edit_item;

	ObjectPropertiesWindowBase* propertyWindow = getParentContainerWindow();
	if(propertyWindow) {
		propertyWindow->Update();
	}
}

void ContainerItemButton::setItem(Item* item)
{
	edit_item = item;
	if(edit_item) {
		SetSprite(edit_item->getClientID());
	} else {
		SetSprite(0);
	}
}

ObjectPropertiesWindowBase* ContainerItemButton::getParentContainerWindow()
{
	for(wxWindow* window = GetParent(); window != nullptr; window = window->GetParent()) {
		ObjectPropertiesWindowBase* propertyWindow = dynamic_cast<ObjectPropertiesWindowBase*>(window);
		if(propertyWindow) {
			return propertyWindow;
		}
	}
	return nullptr;
}

Container* ContainerItemButton::getParentContainer()
{
	ObjectPropertiesWindowBase* propertyWindow = getParentContainerWindow();
	if(propertyWindow) {
		return dynamic_cast<Container*>(propertyWindow->getItemBeingEdited());
	}
	return nullptr;
}

// ContainerItemPopupMenu
ContainerItemPopupMenu::ContainerItemPopupMenu() : wxMenu(wxT(""))
{
	////
}

ContainerItemPopupMenu::~ContainerItemPopupMenu()
{
	////
}

void ContainerItemPopupMenu::Update(ContainerItemButton* btn)
{
	// Clear the menu of all items
	while(GetMenuItemCount() != 0) {
		wxMenuItem* m_item = FindItemByPosition(0);
		// If you add a submenu, this won't delete it.
		Delete(m_item);
	}

	wxMenuItem* addItem = nullptr;
	if(btn->edit_item) {
		Append( CONTAINER_POPUP_MENU_EDIT, wxT("&Edit Item"), wxT("Open the properties menu for this item"));
		addItem = Append( CONTAINER_POPUP_MENU_ADD, wxT("&Add Item"), wxT("Add a newd item to the container"));
		Append( CONTAINER_POPUP_MENU_REMOVE, wxT("&Remove Item"), wxT("Remove this item from the container"));
	} else {
		addItem = Append( CONTAINER_POPUP_MENU_ADD, wxT("&Add Item"), wxT("Add a newd item to the container"));
	}

	Container* parentContainer = btn->getParentContainer();
	if(parentContainer->getVolume() <= (int)parentContainer->getVector().size())
		addItem->Enable(false);
}
