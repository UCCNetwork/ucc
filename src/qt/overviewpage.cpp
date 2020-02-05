// Copyright (c) 2011-2014 The Bitcoin developers
// Copyright (c) 2014-2015 The Dash developers
// Copyright (c) 2015-2017 The PIVX developers
// Copyright (c) 2017-2018 The XDNA Core developers
// Copyright (c) 2018-2019 The ESBC Core developers
// Copyright (c) 2018-2020 The UCC Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "overviewpage.h"
#include "ui_overviewpage.h"

#include "bitcoinunits.h"
#include "clientmodel.h"
#include "guiconstants.h"
#include "guiutil.h"
#include "init.h"
#include "obfuscation.h"
#include "obfuscationconfig.h"
#include "optionsmodel.h"
#include "transactionfilterproxy.h"
#include "transactiontablemodel.h"
#include "walletmodel.h"
#include "masternodeman.h"
#include "main.h"
#include "chainparams.h"
#include "amount.h"
#include "addressbookpage.h"
#include "rpcblockchain.cpp"

#include <QAbstractItemDelegate>
#include <QPainter>
#include <QSettings>
#include <QString>
#include <QTimer>
#include <QUrl>
#include <QDesktopServices>



#define DECORATION_SIZE 38
#define ICON_OFFSET 16
#define NUM_ITEMS 10

extern CWallet* pwalletMain;

class TxViewDelegate : public QAbstractItemDelegate
{
    Q_OBJECT
public:
    TxViewDelegate() : QAbstractItemDelegate(), unit(BitcoinUnits::UCC)
    {
    }

    inline void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
    {
        QSettings settings;
        QString theme = settings.value("theme", "dblue").toString();

        painter->save();

        QIcon icon = qvariant_cast<QIcon>(index.data(Qt::DecorationRole));
        QRect mainRect = option.rect;
        mainRect.moveLeft(ICON_OFFSET);
        QRect decorationRect(mainRect.topLeft(), QSize(DECORATION_SIZE - 6, DECORATION_SIZE - 6));
        int xspace = DECORATION_SIZE + 8;
        int ypad = 6;
        int halfheight = (mainRect.height() - 2 * ypad) / 2;
        QRect amountRect(mainRect.left() + xspace, mainRect.top() + ypad, mainRect.width() - xspace - ICON_OFFSET, halfheight);
        QRect addressRect(mainRect.left() + xspace, mainRect.top() + ypad + halfheight, mainRect.width() - xspace, halfheight);
        icon.paint(painter, decorationRect);

        QDateTime date = index.data(TransactionTableModel::DateRole).toDateTime();
        QString address = index.data(Qt::DisplayRole).toString();
        qint64 amount = index.data(TransactionTableModel::AmountRole).toLongLong();
        bool confirmed = index.data(TransactionTableModel::ConfirmedRole).toBool();
        //QVariant value = index.data(Qt::ForegroundRole);
        QColor foreground = COLOR_BLACK;

        // paint address
        if (theme.operator==("dark")) foreground = QColor(140, 104, 76); //"#8C684C"
        else if (theme.operator==("dblue")) foreground = QColor(205, 220, 234);
        else foreground = COLOR_BLACK;
        //if (value.canConvert<QBrush>()) {
        //    QBrush brush = qvariant_cast<QBrush>(value);
        //    foreground = brush.color();
        //}
        //if (theme.operator==("dark")) foreground = QColor(144, 144, 144);
        //else if (theme.operator==("dblue")) foreground = QColor(103, 119, 127);
        //else foreground = foreground;
        painter->setPen(foreground);
        QRect boundingRect;
        painter->drawText(addressRect, Qt::AlignLeft | Qt::AlignVCenter, address, &boundingRect);

        if (amount < 0) {
            if (theme.operator==("dark")) foreground = QColor(220, 50, 50); //"#DC3232"
            else if (theme.operator==("dblue")) foreground = QColor(220, 50, 50);
            else foreground = COLOR_NEGATIVE;
        } else if (!confirmed) {
            if (theme.operator==("dark")) foreground = QColor(151, 135, 117); //"#978775"
            else if (theme.operator==("dblue")) foreground = QColor(205, 220, 234);
            else foreground = COLOR_UNCONFIRMED;
        } else {
            if (theme.operator==("dark")) foreground = QColor(240, 216, 174); //"#F0D8AE"
            else if (theme.operator==("dblue")) foreground = QColor(205, 220, 234);
            else foreground = COLOR_BLACK;
        }
        painter->setPen(foreground);
        QString amountText = BitcoinUnits::formatWithUnit(unit, amount, true, BitcoinUnits::separatorAlways);
        if (!confirmed) {
            amountText = QString("[") + amountText + QString("]");
        }
        painter->drawText(amountRect, Qt::AlignRight | Qt::AlignVCenter, amountText);

        // paint date
        if (theme.operator==("dark")) foreground = QColor(240, 216, 174); //"#F0D8AE"
        else if (theme.operator==("dblue")) foreground = QColor(205, 220, 234);
        else foreground = COLOR_BLACK;
        painter->drawText(amountRect, Qt::AlignLeft | Qt::AlignVCenter, GUIUtil::dateTimeStr(date));

        painter->restore();
    }

    inline QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const
    {
        return QSize(DECORATION_SIZE, DECORATION_SIZE);
    }

    int unit;
};
#include "overviewpage.moc"

OverviewPage::OverviewPage(QWidget* parent) : QWidget(parent),
                                              ui(new Ui::OverviewPage),
                                              clientModel(0),
                                              walletModel(0),
                                              currentBalance(-1),
                                              currentUnconfirmedBalance(-1),
                                              currentImmatureBalance(-1),
                                              currentWatchOnlyBalance(-1),
                                              currentWatchUnconfBalance(-1),
                                              currentWatchImmatureBalance(-1),
                                              txdelegate(new TxViewDelegate()),
                                              filter(0)
{
    nDisplayUnit = 0; // just make sure it's not unitialized
    ui->setupUi(this);

    // Recent transactions
    ui->listTransactions->setItemDelegate(txdelegate);
    ui->listTransactions->setIconSize(QSize(DECORATION_SIZE, DECORATION_SIZE));
    ui->listTransactions->setAttribute(Qt::WA_MacShowFocusRect, false);

    connect(ui->listTransactions, SIGNAL(clicked(QModelIndex)), this, SLOT(handleTransactionClicked(QModelIndex)));
    //ui->AdditionalFeatures->setTabEnabled(1,false);

    // init "out of sync" warning labels
    ui->labelWalletStatus->setText("(" + tr("out of sync") + ")");
    ui->labelTransactionsStatus->setText("(" + tr("out of sync") + ")");

    //information block update
    timerinfo_mn = new QTimer(this);
    connect(timerinfo_mn, SIGNAL(timeout()), this, SLOT(updateMasternodeInfo()));
    timerinfo_mn->start(1000);

    timerinfo_blockchain = new QTimer(this);
    connect(timerinfo_blockchain, SIGNAL(timeout()), this, SLOT(updateBlockChainInfo()));
    timerinfo_blockchain->start(1000); //30sec

    // start with displaying the "out of sync" warnings
    showOutOfSyncWarning(true);
}

void OverviewPage::handleTransactionClicked(const QModelIndex& index)
{
    if (filter)
        emit transactionClicked(filter->mapToSource(index));
}

OverviewPage::~OverviewPage()
{
    if (!fLiteMode && !fMasterNode) disconnect(timer, SIGNAL(timeout()), this, SLOT(obfuScationStatus()));
    delete ui;
}

void OverviewPage::setBalance(const CAmount& balance, const CAmount& unconfirmedBalance, const CAmount& immatureBalance, const CAmount& anonymizedBalance, const CAmount& watchOnlyBalance, const CAmount& watchUnconfBalance, const CAmount& watchImmatureBalance)
{
    currentBalance = balance - immatureBalance;
    currentUnconfirmedBalance = unconfirmedBalance;
    currentImmatureBalance = immatureBalance;
    currentAnonymizedBalance = anonymizedBalance;
    currentWatchOnlyBalance = watchOnlyBalance;
    currentWatchUnconfBalance = watchUnconfBalance;
    currentWatchImmatureBalance = watchImmatureBalance;

    // UCC labels

    if(balance != 0)
        ui->labelBalance->setText(BitcoinUnits::floorHtmlWithoutUnit(nDisplayUnit, currentBalance, false, BitcoinUnits::separatorNever));
    ui->labelUnconfirmed->setText(BitcoinUnits::floorHtmlWithoutUnit(nDisplayUnit, unconfirmedBalance, false, BitcoinUnits::separatorNever));
    ui->labelImmature->setText(BitcoinUnits::floorHtmlWithoutUnit(nDisplayUnit, immatureBalance, false, BitcoinUnits::separatorNever));
    //ui->labelAnonymized->setText(BitcoinUnits::floorHtmlWithoutUnit(nDisplayUnit, anonymizedBalance, false, BitcoinUnits::separatorAlways));
    ui->labelTotal->setText(BitcoinUnits::floorHtmlWithoutUnit(nDisplayUnit, currentBalance + unconfirmedBalance + immatureBalance, false, BitcoinUnits::separatorNever));


    // Watchonly labels
      // only show immature (newly mined) balance if it's non-zero, so as not to complicate things
    // for the non-mining users
    bool showImmature = immatureBalance != 0;
    bool showWatchOnlyImmature = watchImmatureBalance != 0;

    // for symmetry reasons also show immature label when the watch-only one is shown
    ui->labelImmature->setVisible(showImmature || showWatchOnlyImmature);
    ui->labelImmatureText->setVisible(showImmature || showWatchOnlyImmature);
    //ui->label_UCC4->setVisible(showImmature || showWatchOnlyImmature);

   // ui->labelWatchImmature->setVisible(showWatchOnlyImmature); // show watch-only immature balance

    updateObfuscationProgress();

    static int cachedTxLocks = 0;

    if (cachedTxLocks != nCompleteTXLocks) {
        cachedTxLocks = nCompleteTXLocks;
        ui->listTransactions->update();
    }
}

// show/hide watch-only labels
void OverviewPage::updateWatchOnlyLabels(bool showWatchOnly)
{
       if (!showWatchOnly) {
       // ui->labelWatchImmature->hide();
    } else {
        ui->labelBalance->setIndent(20);
        ui->labelUnconfirmed->setIndent(20);
        ui->labelImmature->setIndent(20);
        ui->labelTotal->setIndent(20);
    }
}

void OverviewPage::setClientModel(ClientModel* model)
{
    this->clientModel = model;
    if (model) {
        // Show warning if this is a prerelease version
        connect(model, SIGNAL(alertsChanged(QString)), this, SLOT(updateAlerts(QString)));
        updateAlerts(model->getStatusBarWarnings());
    }
}

void OverviewPage::setWalletModel(WalletModel* model)
{
    this->walletModel = model;
    if (model && model->getOptionsModel()) {
        // Set up transaction list
        filter = new TransactionFilterProxy();
        filter->setSourceModel(model->getTransactionTableModel());
        filter->setLimit(NUM_ITEMS);
        filter->setDynamicSortFilter(true);
        filter->setSortRole(Qt::EditRole);
        filter->setShowInactive(false);
        filter->sort(TransactionTableModel::Date, Qt::DescendingOrder);

        ui->listTransactions->setModel(filter);
        ui->listTransactions->setModelColumn(TransactionTableModel::ToAddress);



        //----------
        // Keep up to date with wallet
//        setBalance(model->getBalance(), model->getUnconfirmedBalance(), model->getImmatureBalance(), model->getAnonymizedBalance(),
//        model->getWatchBalance(), model->getWatchUnconfirmedBalance(), model->getWatchImmatureBalance());
        connect(model, SIGNAL(balanceChanged(CAmount, CAmount, CAmount, CAmount, CAmount, CAmount, CAmount)), this, SLOT(setBalance(CAmount, CAmount, CAmount, CAmount, CAmount, CAmount, CAmount)));

        connect(model->getOptionsModel(), SIGNAL(displayUnitChanged(int)), this, SLOT(updateDisplayUnit()));

        // connect(ui->obfuscationAuto, SIGNAL(clicked()), this, SLOT(obfuscationAuto()));
        // connect(ui->obfuscationReset, SIGNAL(clicked()), this, SLOT(obfuscationReset()));
        // connect(ui->toggleObfuscation, SIGNAL(clicked()), this, SLOT(toggleObfuscation()));
        connect(model, SIGNAL(notifyWatchonlyChanged(bool)), this, SLOT(updateWatchOnlyLabels(bool)));
        connect(ui->blabel_UCC, SIGNAL(clicked()), this, SLOT(openMyAddresses()));

    }

    // update the display unit, to not use the default ("UCC")
    updateDisplayUnit();
}

void OverviewPage::updateDisplayUnit()
{
    if (walletModel && walletModel->getOptionsModel()) {
        nDisplayUnit = walletModel->getOptionsModel()->getDisplayUnit();
        if (currentBalance != -1)
            setBalance(currentBalance, currentUnconfirmedBalance, currentImmatureBalance, currentAnonymizedBalance,
                currentWatchOnlyBalance, currentWatchUnconfBalance, currentWatchImmatureBalance);

        // Update txdelegate->unit with the current unit
        txdelegate->unit = nDisplayUnit;

        ui->listTransactions->update();
    }
}

void OverviewPage::updateAlerts(const QString& warnings)
{
  //  this->ui->labelAlerts->setVisible(!warnings.isEmpty());
  //  this->ui->labelAlerts->setText(warnings);
}

double roi1, roi2, roi3;

void OverviewPage::updateMasternodeInfo()
{
  int CurrentBlock = clientModel->getNumBlocks();

  if (masternodeSync.IsBlockchainSynced() && masternodeSync.IsSynced())
  {

   int mn1=0;
   int mn2=0;
   int mn3=0;
   int totalmn=0;
   std::vector<CMasternode> vMasternodes = mnodeman.GetFullMasternodeMap();
    for(auto& mn : vMasternodes)
    {
       switch ( mn.Level())
       {
           case 1:
           mn1++;break;
           case 2:
           mn2++;break;
           case 3:
           mn3++;break;
       }

    }
    totalmn=mn1+mn2+mn3;
    ui->labelMnTotal_Value->setText(QString::number(totalmn));
    int maxMnValue = std::max( { mn1, mn2, mn3 }, [](const int& s1, const int& s2) { return s1 < s2; });

    ui->graphMN1->setMaximum(maxMnValue);
    ui->graphMN2->setMaximum(maxMnValue);
    ui->graphMN3->setMaximum(maxMnValue);
    
    ui->graphMN1->setValue(mn1);
    ui->graphMN2->setValue(mn2);
    ui->graphMN3->setValue(mn3);

    // TODO: need a read actual 24h blockcount from chain
    int BlockCount24h = block24hCount > 0 ? block24hCount : 1440;

    // update ROI
    double BlockReward = GetBlockValue(CurrentBlock);
    BlockReward -= BlockReward * Params().GetDevFee() / 100;
	  BlockReward -= BlockReward * Params().GetFundFee() / 100;
    (mn1==0) ? roi1 = 0 : roi1 = (GetMasternodePayment(CurrentBlock, 1, BlockReward)*BlockCount24h)/mn1/COIN;
    (mn2==0) ? roi2 = 0 : roi2 = (GetMasternodePayment(CurrentBlock, 2, BlockReward)*BlockCount24h)/mn2/COIN;
    (mn3==0) ? roi3 = 0 : roi3 = (GetMasternodePayment(CurrentBlock, 3, BlockReward)*BlockCount24h)/mn3/COIN;
    
    if (CurrentBlock >= 0) {
        ui->roi_11->setText(mn1==0 ? "-" : QString::number(roi1,'f',0).append("  |"));
        ui->roi_21->setText(mn2==0 ? "-" : QString::number(roi2,'f',0).append("  |"));
        ui->roi_31->setText(mn3==0 ? "-" : QString::number(roi3,'f',0).append("  |"));

        ui->roi_12->setText(mn1==0 ? " " : QString::number( 1000/roi1,'f',1).append(" days"));
        ui->roi_22->setText(mn2==0 ? " " : QString::number( 3000/roi2,'f',1).append(" days"));
        ui->roi_32->setText(mn3==0 ? " " : QString::number( 5000/roi3,'f',1).append(" days"));
    }
    CAmount tNodesSumm = mn1*1000 + mn2*3000 + mn3*5000;
    CAmount tMoneySupply = chainActive.Tip()->nMoneySupply;
    double tLocked = tMoneySupply > 0 ? 100 * static_cast<double>(tNodesSumm) / static_cast<double>(tMoneySupply / COIN) : 0;
    ui->label_LockedCoin_value->setText(QString::number(tNodesSumm).append(" (" + QString::number(tLocked,'f',1) + "%)"));

    // update timer
    if (timerinfo_mn->interval() == 1000)
            timerinfo_mn->setInterval(10000);
  }

  // update collateral info
  if (CurrentBlock >= 0) {
      ui->label_lcolat->setText("1000 UCC");
      ui->label_mcolat->setText("3000 UCC");
      ui->label_fcolat->setText("5000 UCC");
  }

}

void OverviewPage::updateBlockChainInfo()
{
    if (masternodeSync.IsBlockchainSynced())
    {
        int CurrentBlock = clientModel->getNumBlocks();
        int64_t netHashRate = chainActive.GetNetworkHashPS(24, CurrentBlock);
        double BlockReward = GetBlockValue(CurrentBlock);
        double BlockRewarducc =  static_cast<double>(BlockReward/COIN);
		double CurrentDiff = GetDifficulty();
		double DevFee = Params().GetDevFee();
        double FundFee = Params().GetFundFee();
		
        ui->label_CurrentBlock_value->setText(QString::number(CurrentBlock));

        ui->label_Nethash->setText(tr("Difficulty:"));
        ui->label_Nethash_value->setText(QString::number(CurrentDiff,'f',4));

        ui->label_CurrentBlockReward_value->setText(QString::number(BlockRewarducc, 'f', 1).append(" | ") + QString::number(DevFee).append("% | ") + QString::number(FundFee).append("%"));

        ui->label_Supply_value->setText(QString::number(chainActive.Tip()->nMoneySupply / COIN).append(" UCC"));

        ui->label_24hBlock_value->setText(QString::number(block24hCount));
        ui->label_24hPoS_value->setText(QString::number(static_cast<double>(posMin)/COIN,'f',1).append(" | ") + QString::number(static_cast<double>(posMax)/COIN,'f',1));
        ui->label_24hPoSMedian_value->setText(QString::number(static_cast<double>(posMedian)/COIN,'f',1));
    }
}

void OverviewPage::openMyAddresses()
{
    AddressBookPage* dlg = new AddressBookPage(AddressBookPage::ForEditing, AddressBookPage::ReceivingTab, this);
    dlg->setAttribute(Qt::WA_DeleteOnClose);
    dlg->setModel(walletModel->getAddressTableModel());
    dlg->show();
}

void OverviewPage::showOutOfSyncWarning(bool fShow)
{
    ui->labelWalletStatus->setVisible(fShow);
    //ui->labelObfuscationSyncStatus->setVisible(fShow);
    ui->labelTransactionsStatus->setVisible(fShow);
}

void OverviewPage::updateObfuscationProgress()
{
    return;
}


void OverviewPage::obfuScationStatus()
{
    return;
}

void OverviewPage::obfuscationAuto()
{
    return;
}

void OverviewPage::obfuscationReset()
{
    return;
}

void OverviewPage::toggleObfuscation()
{
    return;
}
