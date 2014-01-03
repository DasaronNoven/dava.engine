/*==================================================================================
    Copyright (c) 2008, binaryzebra
    All rights reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright
    notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
    notice, this list of conditions and the following disclaimer in the
    documentation and/or other materials provided with the distribution.
    * Neither the name of the binaryzebra nor the
    names of its contributors may be used to endorse or promote products
    derived from this software without specific prior written permission.

    THIS SOFTWARE IS PROVIDED BY THE binaryzebra AND CONTRIBUTORS "AS IS" AND
    ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
    WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
    DISCLAIMED. IN NO EVENT SHALL binaryzebra BE LIABLE FOR ANY
    DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
    (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
    LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
    ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
    (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
    SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
=====================================================================================*/


#include "RecentFilesManager.h"
#include "Qt/Settings/SettingsManager.h"


DAVA::Vector<String> RecentFileManager::GetRecentFiles()
{
	DAVA::Vector<String> retVector;
	VariantType recentFilesVariant = SettingsManager::Instance()->GetValue("recentFiles", SettingsManager::INTERNAL);
	if(recentFilesVariant.GetType() == DAVA::VariantType::TYPE_KEYED_ARCHIVE)
	{
		KeyedArchive* archiveRecentFiles = recentFilesVariant.AsKeyedArchive();
		DAVA::int32 size = archiveRecentFiles->Count();
		retVector.resize(size);
		for (DAVA::uint32 i = 0; i < size; ++i)
		{
			retVector[i] = archiveRecentFiles->GetString(Format("%d", i));
		}
		
	}
	return retVector;
}

void RecentFileManager::SetFilesToRecent(DAVA::Vector<String>& fileList)
{
	DAVA::uint32 size = fileList.size() > RECENT_FILES_MAX_COUNT ? RECENT_FILES_MAX_COUNT : fileList.size();
	KeyedArchive* archive = new KeyedArchive();
	for (DAVA::int32 i = 0; i < size; ++i)
	{
		archive->SetString(Format("%d",i), fileList[i]);
	}
	SettingsManager::Instance()->SetValue("recentFiles", DAVA::VariantType(archive), SettingsManager::INTERNAL);
	SafeRelease( archive);
}