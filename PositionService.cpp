// PositionService.cpp : Defines the entry point for the console application.
//

#include "PositionService.h"

using namespace std;

int main(int argc, char* argv[])
{
	// check arguments
	if (argc != 3)
	{
		cout << "Usage: PositionService fillsFilePath pricesFilePath";
		return -1;
	}

	CMessageService messageService;
	CPositionService positionService;
	messageService.Subscribe(&positionService);
	try
	{
		messageService.ReadMessages(argv[1], argv[2]);
		messageService.ProcessPriceMessages();
	}
	catch (const exception& e)
	{
		cout << "Exception:" << e.what();
		return -1;
	}
	catch (...)
	{
		cout << "Unknown exception.";
		return -1;
	}
    return 0;
}

// Make CElementFile working for SFillMessage
std::istream& operator>>(std::istream& is, SFillMessage& element)
{
	is >> element.m_sMessageType >> element.m_nMilliseconds >> element.m_sSymbolName
	   >> element.m_dFillPrice >> element.m_nFillSize >> element.m_sSideIndicator;
	return is;
}

// Make CElementFile working for SPriceMessage
std::istream& operator>>(std::istream& is, SPriceMessage& element)
{
	is >> element.m_sMessageType >> element.m_nMilliseconds >> element.m_sSymbolName
	   >> element.m_dCurrentPrice;
	return is;
}

// Make stdout working for SPnLMessage
std::ostream& operator<<(std::ostream& os, const SPnLMessage& element)
{
	string sep = " ";
	os << element.m_sMessageType << sep << element.m_nMilliseconds << sep << element.m_sSymbolName << sep
	   << element.m_nCurrentSize << sep << element.m_dPnL;
	return os;
}

// Read fill and price update messages from file and store messages in member variables.
void CMessageService::ReadMessages(const std::string& sFillsFile, const std::string& sPricesFile)
{
	CFillMessageFile fillsFile(sFillsFile);
	m_cFillMessages = fillsFile.Read();
	CPriceMessageFile pricesFile(sPricesFile);
	m_cPriceMessages = pricesFile.Read();
}

// Go through each price update message and notify subsriber
void CMessageService::ProcessPriceMessages()
{
	size_t fillMessageIndex{ 0 }; // current index of m_cFillMessages to search for price message timestamp
	for (const SPriceMessage& priceMessage : m_cPriceMessages)
	{
		// get all fill messages until priceMessage timestamp
		long long priceTimestamp = priceMessage.m_nMilliseconds;
		vector<SFillMessage> fillMessages;
		while (fillMessageIndex < m_cFillMessages.size() && m_cFillMessages[fillMessageIndex].m_nMilliseconds <= priceTimestamp)
		{
			fillMessages.push_back(m_cFillMessages[fillMessageIndex++]);
		}
		// notify subscriber
		m_pPositionService->OnUpdatePriceMessage(priceMessage, fillMessages);
	}
}

// Given price update notification, update current positions and output PnL message.
void CPositionService::OnUpdatePriceMessage(const SPriceMessage& priceMessage, const std::vector<SFillMessage>& fillMessages)
{
	// process fill messages
	for (const SFillMessage& fillMessage : fillMessages)
	{
		int size = fillMessage.m_nFillSize;
		double cash = -fillMessage.m_dFillPrice * fillMessage.m_nFillSize;
		if (!fillMessage.IsBuyOrder())
		{
			size = -size;
			cash = -cash;
		}
		string symbol = fillMessage.m_sSymbolName;
		m_cPositions[symbol].first += cash;
		m_cPositions[symbol].second += size;
	}
	// output PnL message give priceMessage
	string priceSymbol = priceMessage.m_sSymbolName;
	SPnLMessage pnlMessage;
	pnlMessage.m_sMessageType = "PNL";
	pnlMessage.m_nMilliseconds = priceMessage.m_nMilliseconds;
	pnlMessage.m_sSymbolName = priceSymbol;
	double positionCash = m_cPositions[priceSymbol].first;
	int positionSize = m_cPositions[priceSymbol].second;
	pnlMessage.m_nCurrentSize = positionSize;
	pnlMessage.m_dPnL = positionSize * priceMessage.m_dCurrentPrice + positionCash;
	cout << pnlMessage << endl;
}
