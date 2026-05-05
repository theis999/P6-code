var __awaiter = (this && this.__awaiter) || function (thisArg, _arguments, P, generator) {
    function adopt(value) { return value instanceof P ? value : new P(function (resolve) { resolve(value); }); }
    return new (P || (P = Promise))(function (resolve, reject) {
        function fulfilled(value) { try { step(generator.next(value)); } catch (e) { reject(e); } }
        function rejected(value) { try { step(generator["throw"](value)); } catch (e) { reject(e); } }
        function step(result) { result.done ? resolve(result.value) : adopt(result.value).then(fulfilled, rejected); }
        step((generator = generator.apply(thisArg, _arguments || [])).next());
    });
};
var __generator = (this && this.__generator) || function (thisArg, body) {
    var _ = { label: 0, sent: function() { if (t[0] & 1) throw t[1]; return t[1]; }, trys: [], ops: [] }, f, y, t, g;
    return g = { next: verb(0), "throw": verb(1), "return": verb(2) }, typeof Symbol === "function" && (g[Symbol.iterator] = function() { return this; }), g;
    function verb(n) { return function (v) { return step([n, v]); }; }
    function step(op) {
        if (f) throw new TypeError("Generator is already executing.");
        while (g && (g = 0, op[0] && (_ = 0)), _) try {
            if (f = 1, y && (t = op[0] & 2 ? y["return"] : op[0] ? y["throw"] || ((t = y["return"]) && t.call(y), 0) : y.next) && !(t = t.call(y, op[1])).done) return t;
            if (y = 0, t) op = [op[0] & 2, t.value];
            switch (op[0]) {
                case 0: case 1: t = op; break;
                case 4: _.label++; return { value: op[1], done: false };
                case 5: _.label++; y = op[1]; op = [0]; continue;
                case 7: op = _.ops.pop(); _.trys.pop(); continue;
                default:
                    if (!(t = _.trys, t = t.length > 0 && t[t.length - 1]) && (op[0] === 6 || op[0] === 2)) { _ = 0; continue; }
                    if (op[0] === 3 && (!t || (op[1] > t[0] && op[1] < t[3]))) { _.label = op[1]; break; }
                    if (op[0] === 6 && _.label < t[1]) { _.label = t[1]; t = op; break; }
                    if (t && _.label < t[2]) { _.label = t[2]; _.ops.push(op); break; }
                    if (t[2]) _.ops.pop();
                    _.trys.pop(); continue;
            }
            op = body.call(thisArg, _);
        } catch (e) { op = [6, e]; y = 0; } finally { f = t = 0; }
        if (op[0] & 5) throw op[1]; return { value: op[0] ? op[1] : void 0, done: true };
    }
};
;
;
var move_array = [];
var evals = [];
var fens = [];
var selected_id = 1;
var host = "https://smakanalysis.head9x.dk";
var evals_dict = {};
function colorMoveTable(idx) {
    if (idx === 0) {
    }
    var movestable = document.getElementById("movestable");
    if (!(movestable instanceof HTMLTableElement)) {
        return;
    }
    var mtrows = movestable.rows;
    for (var i = 1; i < mtrows.length; i++) {
        var r = mtrows.item(i);
        if (r === null || r === void 0 ? void 0 : r.cells) {
            var cellcount = r.cells.length;
            for (var j = 1; j < cellcount; j++) {
                if (r.cells) {
                    r.cells.item(j).classList.remove("cell-select");
                }
            }
        }
    }
    var row = movestable.rows.item(Math.floor((idx + 1) / 2));
    if ((Math.floor(idx + 1) / 2) < 1) {
        movestable.rows.item(2).cells.item(1).classList.remove("cell-select");
        console.error("Illegal index");
        return;
    }
    if (!(row === null || row === void 0 ? void 0 : row.cells)) {
        return;
    }
    var cellno = 1 + (idx + 1) % 2;
    console.log("cellno = " + cellno);
    var target_cell = row.cells.item(cellno);
    target_cell.classList.add("cell-select");
}
function populateMoveArray(mvs) {
    move_array = [];
    mvs.forEach(function (mv) {
        var from_str = mv.slice(0, 2);
        var to_str = mv.slice(2);
        move_array.push((from_str + "-" + to_str));
    });
}
function deleteMoves() {
    var movestable = document.getElementById("movestable");
    if (!(movestable instanceof HTMLTableElement)) {
        return;
    }
    var rows = movestable.rows;
    for (var i = rows.length - 1; i >= 1; i--) {
        movestable.deleteRow(i);
    }
}
function populateMovesTable(moves) {
    var omovestable = document.getElementById("movestable");
    var moveno = 0;
    if (omovestable instanceof HTMLTableElement) {
        for (var i = 0; i < moves.length; i += 2) {
            moveno++;
            var movestable = omovestable.tBodies.item(0);
            var move_w = moves[i].move;
            var newmove = movestable.insertRow();
            var numbercell = newmove.insertCell(0);
            var wcell = newmove.insertCell(1);
            var bcell = newmove.insertCell(2);
            numbercell.innerHTML = moveno.toString() + ".";
            //let btn_w = "<button class='clickable' style='outline: 0; background-color: transparent; border-color: transparent;'>" + move_w +"</button>"
            var btn_w = "<button onclick='jumpToMove(" + i.toString() + ")' class='clickable'>" + move_w + "</button>";
            wcell.innerHTML = btn_w;
            wcell.setAttribute("data-cellid", i.toString());
            if (!(i + 1 >= moves.length)) {
                var move_b = moves[i + 1].move;
                var btn_b = "<button onclick='jumpToMove(" + (i + 1) + ")'class='clickable'>" + move_b + "</input>";
                bcell.innerHTML = btn_b;
                bcell.setAttribute("data-cellid", (i + 1).toString());
            }
        }
    }
}
function getFen(id) {
    return __awaiter(this, void 0, void 0, function () {
        var url, response, result, hadNull_1, movesstrings, error_1;
        return __generator(this, function (_a) {
            switch (_a.label) {
                case 0:
                    url = host + "/games/fen/" + id;
                    _a.label = 1;
                case 1:
                    _a.trys.push([1, 4, , 5]);
                    if (id <= 0) {
                        throw new Error("0 or negative ID's are not allowed.");
                    }
                    return [4 /*yield*/, fetch(url)];
                case 2:
                    response = _a.sent();
                    if (!response.ok) {
                        throw new Error('Response did unnice thing: ${response.status}');
                    }
                    return [4 /*yield*/, response.json()];
                case 3:
                    result = _a.sent();
                    hadNull_1 = false;
                    fens = result.filter(function (x) {
                        if (x === null) {
                            hadNull_1 = true;
                            return false;
                        }
                        return true;
                    });
                    console.log("Had null:", hadNull_1);
                    console.log(result);
                    movesstrings = [];
                    fens.forEach(function (ev) {
                        if (ev)
                            movesstrings.push(ev.move);
                    });
                    populateMoveArray(movesstrings);
                    populateMovesTable(fens);
                    return [3 /*break*/, 5];
                case 4:
                    error_1 = _a.sent();
                    console.log(error_1);
                    return [3 /*break*/, 5];
                case 5: return [2 /*return*/];
            }
        });
    });
}
function updateEvals(request_id) {
    return __awaiter(this, void 0, void 0, function () {
        var url, _i, fens_1, f, item, response, r, error_2;
        return __generator(this, function (_a) {
            switch (_a.label) {
                case 0:
                    url = host + "/fen/eval";
                    console.debug("updateEvals");
                    console.debug(fens);
                    _a.label = 1;
                case 1:
                    _a.trys.push([1, 9, , 10]);
                    _i = 0, fens_1 = fens;
                    _a.label = 2;
                case 2:
                    if (!(_i < fens_1.length)) return [3 /*break*/, 8];
                    f = fens_1[_i];
                    if (f === null)
                        return [3 /*break*/, 7];
                    item = evals_dict[f.fen_string];
                    if (!item) return [3 /*break*/, 3];
                    console.debug("exists");
                    return [3 /*break*/, 6];
                case 3: return [4 /*yield*/, fetch(url, { method: 'POST', body: f.fen_string })];
                case 4:
                    response = _a.sent();
                    if (!response.ok) {
                        throw new Error('Response did unnice thing: ${response.status}');
                    }
                    return [4 /*yield*/, response.json()];
                case 5:
                    r = _a.sent();
                    r[0].ply = f.ply;
                    evals_dict[f.fen_string] = item = r[0];
                    console.log(r);
                    _a.label = 6;
                case 6:
                    //if (request_id != selected_id) break;
                    evals[f.ply - 1] = item;
                    _a.label = 7;
                case 7:
                    _i++;
                    return [3 /*break*/, 2];
                case 8: return [3 /*break*/, 10];
                case 9:
                    error_2 = _a.sent();
                    console.log(error_2);
                    return [3 /*break*/, 10];
                case 10: return [2 /*return*/];
            }
        });
    });
}
function getEval(id) {
    return __awaiter(this, void 0, void 0, function () {
        var url, response, result, movesstrings, error_3;
        return __generator(this, function (_a) {
            switch (_a.label) {
                case 0:
                    url = host + "/games/eval/" + id;
                    _a.label = 1;
                case 1:
                    _a.trys.push([1, 4, , 5]);
                    if (id <= 0) {
                        throw new Error("0 or negative ID's are not allowed.");
                    }
                    return [4 /*yield*/, fetch(url)];
                case 2:
                    response = _a.sent();
                    if (!response.ok) {
                        throw new Error('Response did unnice thing: ${response.status}');
                    }
                    return [4 /*yield*/, response.json()];
                case 3:
                    result = _a.sent();
                    console.log(result);
                    evals = result;
                    movesstrings = [];
                    result.forEach(function (ev) {
                        movesstrings.push(ev.move);
                    });
                    populateMoveArray(movesstrings);
                    return [3 /*break*/, 5];
                case 4:
                    error_3 = _a.sent();
                    console.error(error_3.message);
                    return [3 /*break*/, 5];
                case 5: return [2 /*return*/];
            }
        });
    });
}
function clearEvalTbl() {
    var eval_tbl = document.getElementById("evaltable");
    if (eval_tbl instanceof HTMLTableElement) {
        var tbl_coll = eval_tbl.rows;
        var secondrow = tbl_coll[1];
        var cls = secondrow.cells;
        for (var i = 0; i < cls.length; i++) {
            var cell = cls[i];
            cell.innerHTML = "";
        }
    }
}
function updateEvalTbl(plyno) {
    var eval_tbl = document.getElementById("evaltable");
    if (eval_tbl instanceof HTMLTableElement) {
        var tbl_coll = eval_tbl.rows;
        var secondrow = tbl_coll[1];
        console.log(secondrow);
        var cls = secondrow.cells;
        var a = evals[plyno - 1];
        console.log("eval");
        console.log(a);
        if (!a) {
            return;
        }
        for (var i = 0; i < cls.length; i++) {
            var cell = cls[i];
            cell.style.textAlign = "center";
            switch (cell.id) {
                case "plycol":
                    cell.innerHTML = a.ply.toString();
                    break;
                case "evcol":
                    if (evals[plyno - 1].forced_mate === true) {
                        cell.innerHTML = "M*";
                    }
                    else {
                        cell.innerHTML = a.pawn_eval.toLocaleString('en-us', { maximumFractionDigits: 2, minimumFractionDigits: 2 });
                    }
                    break;
                case "bmcol":
                    cell.innerHTML = a.bestmove;
                    break;
                case "pdcol":
                    cell.innerHTML = a.ponder;
                    break;
                default:
                    break;
            }
        }
    }
}
