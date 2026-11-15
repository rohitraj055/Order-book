// ========================================
// ORDERBOOK - FRONTEND DEMO
// This is temporary JavaScript simulation.
// The actual C++ engine will be connected later.
// ========================================


// DOM ELEMENTS

const orderForm = document.getElementById("order-form");

const orderType = document.getElementById("order-type");
const orderPrice = document.getElementById("order-price");
const orderQuantity = document.getElementById("order-quantity");

const formMessage = document.getElementById("form-message");

const bestBidElement = document.getElementById("best-bid");
const bestAskElement = document.getElementById("best-ask");
const spreadElement = document.getElementById("spread");
const totalTradesElement = document.getElementById("total-trades");

const asksList = document.getElementById("asks-list");
const bidsList = document.getElementById("bids-list");

const tradesList = document.getElementById("trades-list");
const activeOrdersList = document.getElementById("active-orders-list");


// ========================================
// INITIAL DEMO DATA
// ========================================

let nextOrderId = 211;

let totalTrades = 1183032;

let orders = [
    { id: 201, side: "BUY", price: 10004, quantity: 25 },
    { id: 202, side: "SELL", price: 10005, quantity: 12 },
    { id: 203, side: "BUY", price: 10003, quantity: 16 },

    { id: 204, side: "SELL", price: 10006, quantity: 30 },
    { id: 205, side: "SELL", price: 10007, quantity: 18 },
    { id: 206, side: "SELL", price: 10008, quantity: 22 },
    { id: 207, side: "SELL", price: 10009, quantity: 15 },

    { id: 208, side: "BUY", price: 10002, quantity: 32 },
    { id: 209, side: "BUY", price: 10001, quantity: 20 },
    { id: 210, side: "BUY", price: 10000, quantity: 14 }
];

let recentTrades = [
    { buyId: 101, sellId: 102, price: 10004, quantity: 5 },
    { buyId: 103, sellId: 104, price: 10003, quantity: 8 },
    { buyId: 105, sellId: 106, price: 10002, quantity: 3 },
    { buyId: 107, sellId: 108, price: 10004, quantity: 10 },
    { buyId: 109, sellId: 110, price: 10001, quantity: 4 }
];


// ========================================
// HELPER FUNCTIONS
// ========================================

function formatNumber(number) {
    return number.toLocaleString("en-IN");
}


function getSelectedSide() {
    return document.querySelector(
        'input[name="side"]:checked'
    ).value;
}


function getBestBid() {
    const bids = orders.filter(order => order.side === "BUY");

    if (bids.length === 0) return null;

    return Math.max(...bids.map(order => order.price));
}


function getBestAsk() {
    const asks = orders.filter(order => order.side === "SELL");

    if (asks.length === 0) return null;

    return Math.min(...asks.map(order => order.price));
}


// ========================================
// UPDATE MARKET SUMMARY
// ========================================

function updateMarketSummary() {

    const bestBid = getBestBid();
    const bestAsk = getBestAsk();

    bestBidElement.textContent =
        bestBid === null ? "-" : formatNumber(bestBid);

    bestAskElement.textContent =
        bestAsk === null ? "-" : formatNumber(bestAsk);

    spreadElement.textContent =
        bestBid === null || bestAsk === null
            ? "-"
            : formatNumber(bestAsk - bestBid);

    totalTradesElement.textContent =
        formatNumber(totalTrades);
}


// ========================================
// RENDER ORDER BOOK
// ========================================

function renderOrderBook() {

    const asks = orders
        .filter(order => order.side === "SELL")
        .sort((a, b) => a.price - b.price);

    const bids = orders
        .filter(order => order.side === "BUY")
        .sort((a, b) => b.price - a.price);


    // Aggregate orders at identical prices

    function aggregateLevels(sideOrders) {

        const levels = new Map();

        sideOrders.forEach(order => {

            const current = levels.get(order.price) || 0;

            levels.set(
                order.price,
                current + order.quantity
            );

        });

        return [...levels.entries()];
    }


    const askLevels = aggregateLevels(asks).slice(0, 5);
    const bidLevels = aggregateLevels(bids).slice(0, 5);


    function createRows(levels, className) {

        let cumulativeQuantity = 0;

        const quantities = levels.map(level => level[1]);

        const maxQuantity = Math.max(...quantities, 1);

        return levels.map(([price, quantity]) => {

            cumulativeQuantity += quantity;

            const depth = (quantity / maxQuantity) * 100;

            return `
                <div class="book-row ${className}"
                     style="--depth: ${depth}%">

                    <span>${formatNumber(price)}</span>

                    <span>${formatNumber(quantity)}</span>

                    <span>${formatNumber(cumulativeQuantity)}</span>

                </div>
            `;

        }).join("");
    }


    asksList.innerHTML = createRows(askLevels, "ask-row");

    bidsList.innerHTML = createRows(bidLevels, "bid-row");


    // Highlight best prices

    const firstAsk = asksList.querySelector(".book-row");
    const firstBid = bidsList.querySelector(".book-row");

    if (firstAsk) firstAsk.classList.add("best-ask-row");
    if (firstBid) firstBid.classList.add("best-bid-row");


    updateMarketSummary();
}


// ========================================
// RENDER RECENT TRADES
// ========================================

function renderTrades() {

    tradesList.innerHTML = recentTrades.map(trade => {

        return `
            <div class="trade-row">

                <span>#${trade.buyId}</span>

                <span>#${trade.sellId}</span>

                <span>${formatNumber(trade.price)}</span>

                <span>${formatNumber(trade.quantity)}</span>

            </div>
        `;

    }).join("");

}


// ========================================
// RENDER ACTIVE ORDERS
// ========================================

function renderActiveOrders() {

    activeOrdersList.innerHTML = orders.map(order => {

        const sideClass =
            order.side === "BUY" ? "positive" : "negative";

        return `
            <div class="active-row">

                <span>#${order.id}</span>

                <span class="${sideClass}">${order.side}</span>

                <span>${formatNumber(order.price)}</span>

                <span>${formatNumber(order.quantity)}</span>

                <span class="order-status">RESTING</span>

            </div>
        `;

    }).join("");

}


// ========================================
// EXECUTE MATCHING
// ========================================

function matchOrder(incoming) {

    const oppositeSide =
        incoming.side === "BUY" ? "SELL" : "BUY";


    while (incoming.quantity > 0) {

        const oppositeOrders = orders
            .filter(order => order.side === oppositeSide)
            .sort((a, b) => {

                if (a.price !== b.price) {

                    return incoming.side === "BUY"
                        ? a.price - b.price
                        : b.price - a.price;

                }

                return a.id - b.id;

            });


        if (oppositeOrders.length === 0) break;


        const restingOrder = oppositeOrders[0];


        const canMatch =
            incoming.side === "BUY"
                ? incoming.price >= restingOrder.price
                : incoming.price <= restingOrder.price;


        if (!canMatch) break;


        const tradedQuantity = Math.min(
            incoming.quantity,
            restingOrder.quantity
        );


        const buyId =
            incoming.side === "BUY"
                ? incoming.id
                : restingOrder.id;


        const sellId =
            incoming.side === "SELL"
                ? incoming.id
                : restingOrder.id;


        recentTrades.unshift({

            buyId,
            sellId,

            price: restingOrder.price,

            quantity: tradedQuantity

        });


        totalTrades++;

        incoming.quantity -= tradedQuantity;

        restingOrder.quantity -= tradedQuantity;


        // Remove fully filled resting order

        if (restingOrder.quantity === 0) {

            orders = orders.filter(
                order => order.id !== restingOrder.id
            );

        }

    }


    // Limit orders can rest in the book.
    // Market orders must never rest.

    if (
        incoming.quantity > 0 &&
        incoming.type === "LIMIT"
    ) {

        orders.push({

            id: incoming.id,

            side: incoming.side,

            price: incoming.price,

            quantity: incoming.quantity

        });

    }


    recentTrades = recentTrades.slice(0, 15);

}


// ========================================
// LIMIT / MARKET PRICE FIELD
// ========================================

function updatePriceField() {

    const isMarketOrder = orderType.value === "MARKET";

    orderPrice.disabled = isMarketOrder;

    orderPrice.required = !isMarketOrder;

    orderPrice.placeholder = isMarketOrder
        ? "Not required for market order"
        : "Enter limit price";

    orderPrice.value = "";

}


orderType.addEventListener("change", updatePriceField);


// ========================================
// SUBMIT ORDER
// ========================================

orderForm.addEventListener("submit", function(event) {

    event.preventDefault();


    const type = orderType.value;

    const side = getSelectedSide();

    const quantity = Number(orderQuantity.value);

    const price = Number(orderPrice.value);


    if (!Number.isInteger(quantity) || quantity <= 0) {

        showMessage("Enter a valid positive quantity.", "error");

        return;

    }


    if (
        type === "LIMIT" &&
        (!Number.isInteger(price) || price <= 0)
    ) {

        showMessage("Enter a valid positive limit price.", "error");

        return;

    }


    const id = nextOrderId++;


    let order = {

        id,

        side,

        price,

        quantity,

        type

    };


    // Market orders use an unrestricted matching price.

    if (type === "MARKET") {

        const bestPrice =
            side === "BUY" ? getBestAsk() : getBestBid();

        if (bestPrice === null) {

            showMessage("No liquidity available for this market order.", "error");

            return;

        }

        order.price = side === "BUY"
            ? Infinity
            : -Infinity;

    }


    matchOrder(order);


    renderOrderBook();

    renderTrades();

    renderActiveOrders();


    showMessage(
        `Order #${id} submitted successfully in demo mode.`,
        "success"
    );


    orderForm.reset();

    updatePriceField();

});


// ========================================
// MESSAGE DISPLAY
// ========================================

function showMessage(message, type) {

    formMessage.textContent = message;

    formMessage.style.color =
        type === "error" ? "#f05b67" : "#00c087";

}


// ========================================
// INITIAL RENDER
// ========================================

updatePriceField();

renderOrderBook();

renderTrades();

renderActiveOrders();