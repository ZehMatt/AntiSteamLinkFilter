#include "BinaryPattern.h"
#include <windows.h>

bool testPattern(const void* pMemory, uint32_t nMaxBuf, const char *pszPattern)
{
	const uint8_t *pCurBuf = (uint8_t*)pMemory;
	const uint8_t *pEnd = (pCurBuf + nMaxBuf);

	const char *pCurPattern = pszPattern;
	char cbTemp[3] = { 0 };

	bool bWildcard = false;

	bool bOptional = false;
	bool bOptionalMatch = false;

	bool bOptionBlock = false;
	bool bOptionBlockMatch = false;

	uint8_t cSearchByte = 0;
	const uint8_t *pOptionalBuf = NULL;

	while (pCurBuf < pEnd && *pCurPattern != '\0')
	{
		// Remove whitespace.
		while (*pCurPattern == ' ')
			pCurPattern++;

		// Test optional case
		if (*pCurPattern == '[')
		{
			bOptional = true;
			pCurPattern++;

			pOptionalBuf = pCurBuf;
			bOptionalMatch = true;

			continue;
		}
		else if (*pCurPattern == ']')
		{
			bOptional = false;
			pCurPattern++;

			if (!bOptionalMatch)
				pCurBuf = pOptionalBuf;

			continue;
		}

		// Option block, must match any of given.
		if (*pCurPattern == '<')
		{
			bOptionBlock = true;
			pCurPattern++;

			continue;
		}
		else if (*pCurPattern == '>')
		{
			// Test if anything inside the block matched.
			if (!bOptionBlockMatch)
				return false;

			// Reset.
			bOptionBlockMatch = false;
			bOptionBlock = false;

			pCurPattern++;

			continue;
		}
		else if (*pCurPattern == '|')
		{
			if (!bOptionBlock)
				return false;	// Invalid syntax.

			pCurPattern++;
			continue;
		}

		// Wildcards
		else if (*pCurPattern == '*')
		{
			bWildcard = true;

			pCurPattern++;

			continue;
		}

		// Normal byte pattern.
		else if (*pCurPattern == '?' && *(pCurPattern + 1) == '?')
		{
			pCurBuf++;
			pCurPattern += 2;

			continue;
		}
		else
		{
			cbTemp[0] = *(pCurPattern);
			cbTemp[1] = *(pCurPattern + 1);
			cSearchByte = (uint8_t)strtoul(cbTemp, NULL, 16);

			if (!bWildcard)
				pCurPattern += 2; // Skip byte
		}

		if (*pCurBuf != cSearchByte)
		{
			if (bWildcard)
			{
				// Currently anything can match, continue.
				pCurBuf++;
				continue;
			}
			else if (bOptionBlock)
			{
				// We don't continue in buffer here, optional.
				pCurBuf++;
				continue;
			}
			else if (bOptional)
			{
				bOptionalMatch = false;
				pCurBuf++;
				continue;
			}

			return false;
		}
		else
		{
			if (bWildcard)
			{
				bWildcard = false;

				pCurPattern += 2;
			}
			else if (bOptionBlock)
			{
				bOptionBlockMatch = true;

				// We don't have to test the other cases, ignore them.
				while (*pCurPattern != '>')
					pCurPattern++;
			}
		}

		pCurBuf++;
	}

	return true;
}

bool findPatternInModule(void *hMod, const char *pattern, bool scanCodeOnly, uintptr_t& virtualAddress)
{
	virtualAddress = 0;

	const uint8_t *imageData = reinterpret_cast<const uint8_t*>(hMod);

	const IMAGE_DOS_HEADER *dosHeader = reinterpret_cast<const IMAGE_DOS_HEADER*>(imageData);
	const IMAGE_NT_HEADERS *ntHeaders = reinterpret_cast<const IMAGE_NT_HEADERS*>(imageData + dosHeader->e_lfanew);
	const IMAGE_SECTION_HEADER *firstSection = reinterpret_cast<const IMAGE_SECTION_HEADER*>(imageData + dosHeader->e_lfanew + sizeof(IMAGE_NT_HEADERS));

	const IMAGE_SECTION_HEADER *curSectionHeader = firstSection;
	for (uint16_t numSection = 0; numSection < ntHeaders->FileHeader.NumberOfSections; numSection++, curSectionHeader++)
	{
		if (scanCodeOnly)
		{
			if (!!(curSectionHeader->Characteristics & IMAGE_SCN_CNT_CODE) == false &&
				!!(curSectionHeader->Characteristics & IMAGE_SCN_MEM_EXECUTE) == false)
			{
				continue;
			}
		}

		size_t dataOffset = curSectionHeader->PointerToRawData;
		size_t maxDataOffset = curSectionHeader->SizeOfRawData;

		for (size_t offset = dataOffset; offset < maxDataOffset; offset++)
		{
			const uint8_t *sectionData = imageData + offset;

			if (testPattern(sectionData, maxDataOffset - offset, pattern) == true)
			{
				virtualAddress = reinterpret_cast<size_t>(imageData + offset);
				return true;
			}
		}
	}

	return false;
}