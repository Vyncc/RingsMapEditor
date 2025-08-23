#pragma once

#include <wtypes.h>
#include <Psapi.h>

// All GetInstanceOf methods go through the entire GObjects array, so it's not recommended you use them for everything.
// Get the default constructor of a class.
template<class U>
U* GetDefaultInstanceOf()
{
	UClass* objectClass = (UClass*)U::StaticClass();

	for (int i = (UObject::GObjObjects()->size() - 1000); i > 0; i--)
	{
		UObject* checkObject = UObject::GObjObjects()->at(i);

		if (checkObject && checkObject->IsA(objectClass))
		{
			if (std::string(checkObject->GetFullName()).find("Default") != std::string::npos)
				return (U*)checkObject;
		}
	}

	return nullptr;
}


// Get the most current/active instance of a class. Example: UEngine* engine = GetInstanceOf<UEngine>();
template<class U>
U* GetInstanceOf()
{
	UClass* objectClass = (UClass*)U::StaticClass();

	for (int i = (UObject::GObjObjects()->size() - 1000); i > 0; i--)
	{
		UObject* checkObject = UObject::GObjObjects()->at(i);

		if (checkObject && checkObject->IsA(objectClass))
		{
			
			if (std::string(checkObject->GetFullName()).find("Default") == std::string::npos && !(checkObject->ObjectFlags & EObjectFlags::RF_ClassDefaultObject))
				return (U*)checkObject;
		}
	}

	return nullptr;
}

// Get the most current/active instance of a class. Example: UEngine* engine = GetInstanceOf<UEngine>();
template<class U>
U* GetInstanceOf(std::string keyword)
{
	UClass* baseClass = (UClass*)U::StaticClass();

	for (UObject* uObject : *UObject::GObjObjects())
	{
		if (uObject)
		{
			std::string objectName = uObject->GetName();
			std::string objectFullName = uObject->GetFullName();

			if (objectName.find(keyword) != std::string::npos
				&& objectFullName.find("Class ") == std::string::npos
				&& objectFullName.find("Function ") == std::string::npos
				&& objectFullName.find("Property") == std::string::npos
				&& objectFullName.find("Const ") == std::string::npos
				&& objectFullName.find("Enum ") == std::string::npos
				&& objectFullName.find("State ") == std::string::npos
				&& objectFullName.find("ScriptStruct ") == std::string::npos
				&& objectFullName.find("ScriptStruct ") == std::string::npos
				&& objectFullName.find("Default__") == std::string::npos
				&& objectFullName.find("Archetypes.") == std::string::npos)
			{
				if (uObject->IsA(baseClass))
				{
					return (U*)uObject;
				}
			}
		}
	}

	return nullptr;
}

// Get the most current/active instance of a class. Example: UEngine* engine = GetInstanceOf<UEngine>();
template<class U>
U* GetInstanceOfByFullName(std::string exactFullName)
{
	UClass* baseClass = (UClass*)U::StaticClass();

	for (UObject* uObject : *UObject::GObjObjects())
	{
		if (uObject)
		{
			if (uObject->GetFullName() == exactFullName)
			{
				if (uObject->IsA(baseClass))
				{
					return (U*)uObject;
				}
			}
		}
	}

	return nullptr;
}

// Get the most current/active instance of a class. Example: UEngine* engine = GetInstanceOf<UEngine>();
template<class U>
TArray<U*> GetAllInstancesOf(std::string keyword)
{
	TArray<U*> objectInstances;
	UClass* baseClass = (UClass*)U::StaticClass();

	for (int i = (UObject::GObjObjects()->size() - 1000); i > 0; i--)
	{
		UObject* uObject = UObject::GObjObjects()->at(i);
		if (uObject)
		{
			std::string objectFullName = uObject->GetFullName();

			if (objectFullName.find(keyword) != std::string::npos
				&& objectFullName.find("Class ") == std::string::npos
				&& objectFullName.find("Function ") == std::string::npos
				&& objectFullName.find("Property") == std::string::npos
				&& objectFullName.find("Const ") == std::string::npos
				&& objectFullName.find("Enum ") == std::string::npos
				&& objectFullName.find("State ") == std::string::npos
				&& objectFullName.find("ScriptStruct ") == std::string::npos
				&& objectFullName.find("ScriptStruct ") == std::string::npos
				&& objectFullName.find("Default__") == std::string::npos
				&& objectFullName.find("Archetypes.") == std::string::npos)
			{
				if (uObject->IsA(baseClass))
				{
					objectInstances.Add((U*)uObject);
				}
			}
		}
	}

	return objectInstances;
}

// Get all active instances of a class type.
template<class U>
TArray<U*> GetAllInstancesOf()
{
	TArray<U*> objectInstances;
	UClass* baseClass = (UClass*)U::StaticClass();

	for (int i = (UObject::GObjObjects()->size() - 1000); i > 0; i--)
	{
		UObject* checkObject = UObject::GObjObjects()->at(i);

		if (checkObject && checkObject->IsA(baseClass))
		{
			if (std::string(checkObject->GetFullName()).find("Default") == std::string::npos)
				objectInstances.Add((U*)checkObject);
		}
	}

	return objectInstances;
}

// Get all default instances of a class type.
template<class U>
TArray<U*> GetAllDefaultInstancesOf()
{
	TArray<U*> objectInstances;
	UClass* baseClass = (UClass*)U::StaticClass();

	for (int i = 0; i < (UObject::GObjObjects()->size() - 1000); i++)
	{
		UObject* checkObject = UObject::GObjObjects()->at(i);

		if (checkObject && checkObject->IsA(baseClass))
		{
			if (std::string(checkObject->GetFullName()).find("Default") != std::string::npos)
				objectInstances.Add((U*)checkObject);
		}
	}

	return objectInstances;
}

// Get an object instance by it's name and class. Example: UMaterialInstanceConstant* octaneMIC = StaticLoadObject<UMaterialInstanceConstant>("Body_Octane.Body_Octane_MIC");
template<class U>
U* StaticLoadObject(const std::string& objectName)
{
	UClass* objectClass = (UClass*)U::StaticClass();

	for (int i = (UObject::GObjObjects()->size() - 1000); i > 0; i--)
	{
		UObject* checkObject = UObject::GObjObjects()->at(i);

		if (checkObject && checkObject->IsA(objectClass))
		{
			std::string checkFullName = std::string(checkObject->GetFullName());

			if (checkFullName == objectName || checkFullName.find(objectName) != std::string::npos)
				return (U*)checkObject;
		}
	}

	return nullptr;
}

// Creates a new transient instance of a class which then adds it to globals.
// YOU are required to make sure these objects eventually get eaten up by the garbage collector in some shape or form.
// Example: UObject* newObject = CreateInstance<UObject>();|
inline UScriptGroup_ORS* scriptORS = nullptr;
inline std::vector<class UObject*> CreatedInstances;

inline void MarkInvincible(class UObject* object)
{
	if (object)
	{
		object->ObjectFlags &= ~EObjectFlags::RF_Transient;
		object->ObjectFlags |= EObjectFlags::RF_Public;
		object->ObjectFlags |= EObjectFlags::RF_Standalone;
		object->ObjectFlags |= EObjectFlags::RF_MarkAsRootSet;
	}
}

inline bool IsInvincible(class UObject* object)
{
	if (object)
	{
		return (object->ObjectFlags & EObjectFlags::RF_Transient) == 0 &&
			(object->ObjectFlags & EObjectFlags::RF_Public) != 0 &&
			(object->ObjectFlags & EObjectFlags::RF_Standalone) != 0 &&
			(object->ObjectFlags & EObjectFlags::RF_MarkAsRootSet) != 0;
	}

	return false;
}

inline void MarkForDestory(class UObject* object)
{
	if (object)
	{
		object->ObjectFlags = 0;
		object->ObjectFlags = EObjectFlags::RF_Transient;
		object->MarkPendingKill();
	}
}

template<typename T>
T* CreateInstance()
{
	if (!scriptORS)
	{
		scriptORS = GetInstanceOf<UScriptGroup_ORS>();

		if (!scriptORS)
		{
			return nullptr;
		}
	}

	T* returnObject = nullptr;

	if (std::is_base_of<UObject, T>::value)
	{
		//T* defaultObject = GetDefaultInstanceOf<T>();
		UClass* staticClass = T::StaticClass();
		UObject* outer = staticClass->Outer;

		if (staticClass)
		{
			returnObject = static_cast<T*>(scriptORS->CreateObject(staticClass, outer));
		}

		// Making sure newly created object doesn't get randomly destroyed by the garbage collector when we don't want it do.
		if (returnObject)
		{
			MarkInvincible(returnObject);
			AddInstance(returnObject);
		}
	}

	return returnObject;
}

inline void AddInstance(UObject* object)
{
	if (object)
	{
		CreatedInstances.push_back(object);
	}
}

inline void DestroyInstance(UObject* object)
{
	if (scriptORS)
	{
		if (object)
		{
			auto it = std::find(CreatedInstances.begin(), CreatedInstances.end(), object);
			if (it != CreatedInstances.end())
			{
				LOG("Destroying object: {} | {}", object->ObjectInternalInteger, object->GetFullName());
				scriptORS->DestroyObject(object);
				CreatedInstances.erase(it);
			}
			else
			{
				LOG("Object not found in CreatedInstances: {}", object->GetFullName());
			}
		}
		else
		{
			LOG("Object is null, cannot destroy.");
		}
	}
	else
	{
		LOG("scriptORS is null, cannot destroy object: {}", object ? object->GetFullName() : "NULL");
	}
}

inline FString StringToFString(const std::string& s)
{
	wchar_t* p = new wchar_t[s.size() + 1];
	for (std::string::size_type i = 0; i < s.size(); ++i)
		p[i] = s[i];

	p[s.size()] = '\0';
	return FString(p);
}

inline uintptr_t FindPattern(HMODULE module, const uint8_t* pattern, const char* mask, int offset)
{
	MODULEINFO info = { };
	GetModuleInformation(GetCurrentProcess(), module, &info, sizeof(MODULEINFO));

	uintptr_t start = reinterpret_cast<uintptr_t>(module);
	size_t length = info.SizeOfImage;

	size_t pos = 0;
	size_t maskLength = std::strlen(mask) - 1;

	for (uintptr_t retAddress = start; retAddress < start + length; retAddress++)
	{
		if (*reinterpret_cast<unsigned char*>(retAddress) == pattern[pos] || mask[pos] == '?')
		{
			if (pos == maskLength)
			{
				uintptr_t addrWithOffset = retAddress - (maskLength - 1);

				if (offset != 0) {
					addrWithOffset += *(int*)addrWithOffset + 4 + offset;
					addrWithOffset += *(int*)(addrWithOffset + 3) + 7;
				}

				return addrWithOffset;
			}

			pos++;
		}
		else
		{
			retAddress -= pos;
			pos = 0;
		}
	}

	return NULL;
}

inline std::string Hex(uintptr_t address, uint64_t width)
{
	std::ostringstream stream;
	stream << "0x" << std::setfill('0') << std::setw(width) << std::right << std::uppercase << std::hex << address;
	return stream.str();
}

inline void InitFindFunction()
{
	UFunction::FindFunction("");
}

inline FName GetFNameByString(const std::string& name)
{
	static bool mapInitialized = false;
	static std::map<const std::string, FNameEntry*> fnameStringMap{};

	if (!mapInitialized)
	{
		for (FNameEntry* fnameEntry : *GNames)
		{
			if (fnameEntry)
			{
				const std::string fnameString = fnameEntry->ToString();
				fnameStringMap[fnameString] = fnameEntry;
			}
		}

		mapInitialized = true;
	}

	if (fnameStringMap.contains(name))
	{
		return FName(fnameStringMap[name]->Index);
	}
	else
	{
		return FName(0);
	}
}

inline FName GetFNameByIndex(const int& index)
{
	static bool mapInitialized = false;
	static std::map<const int, FNameEntry*> fnameIndexMap{};

	if (!mapInitialized)
	{
		for (FNameEntry* fnameEntry : *GNames)
		{
			if (fnameEntry)
			{
				const int fnameIndex = fnameEntry->Index;
				fnameIndexMap[fnameIndex] = fnameEntry;
			}
		}

		mapInitialized = true;
	}

	if (fnameIndexMap.contains(index))
	{
		return FName(index);
	}
	else
	{
		return FName(0);
	}
}

inline void InitRLSDK(bool log = false)
{
	//uintptr_t GObjectsAddress = FindPattern(GetModuleHandle(NULL), (uint8_t*)"\xE8\x00\x00\x00\x00\x8B\x5D\xAF", (char*)"x????xxx", 0x65); //this signature doesn't work since v2.46 (04/12/2024)
	//uintptr_t GNamesAddress = FindPattern(GetModuleHandle(NULL), (uint8_t*)"\xE8\x00\x00\x00\x00\x48\xC7\xC7", (char*)"x????xxx", 0x2f); //this signature doesn't work since v2.46 (04/12/2024)

	uintptr_t EntryPointAddress = reinterpret_cast<uintptr_t>(GetModuleHandle(NULL));
	uintptr_t GObjectsAddress = EntryPointAddress + 0x23C1418; //v2.55 offset
	uintptr_t GNamesAddress = EntryPointAddress + 0x23C13D0; //v2.55 offset

	GObjects = reinterpret_cast<TArray<UObject*>*>(GObjectsAddress);
	GNames = reinterpret_cast<TArray<FNameEntry*>*>(GNamesAddress);

	if (log)
	{
		if (GObjects)
		{
			LOG("GObjectsAddress : {}", Hex(GObjectsAddress, sizeof(GObjectsAddress)));
		}
		else
		{
			LOG("GObject NULL !");
		}

		if (GNames)
		{
			LOG("GNamesAddress : {}", Hex(GNamesAddress, sizeof(GNamesAddress)));
		}
		else
		{
			LOG("GNames NULL !");
		}
	}

	InitFindFunction();
}