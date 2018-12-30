#!/usr/bin/node

const argv = require("yargs").argv;
const child_process = require("child_process");
const fs = require("fs");
require('colors');

function create_data(test_suite) {
	console.log("建立測試資料...");
	for (let test of test_suite) {
		test.create_data();
	}
}

function create_answer(test_suite) {
	for (let test of test_suite) {
		child_process.execSync(`mkdir -p standard_answer/${test.directory}`);
		fs.writeFileSync(`standard_answer/${test.directory}/status`, test.status.answer);
		if (typeof test.commit.answer == "function") {
			test.commit.answer();
		} else if (test.commit.not_exist != true) {
			fs.writeFileSync(`standard_answer/${test.directory}/commit`, test.commit.answer);
		}
		fs.writeFileSync(`standard_answer/${test.directory}/log`, test.log.answer);
	}
}

// 時限爲五秒
const TIME_LIMIT = 5000;

// 回傳值爲一個布林值，表示是否通過所有測資
function judge_stdout(test_suite, subcommand) {

	const total = test_suite.length;

	let passed = 0;

	console.log(`----------------------- 測試 ${subcommand} 子指令 --------------------------`);
	console.log(`共有 ${total} 項測資\n`);
	for (let test of test_suite) {
		console.log(test.description);
		child_process.execSync(`mkdir -p your_answer/${test.directory}`);

		try {
			switch (subcommand) {
				case "status":
					child_process.execSync(
						`./loser ${test.status.alias} data/${test.directory} > your_answer/${test.directory}/${subcommand}`,
						{ timeout: TIME_LIMIT });
					break;
				case "log":
					child_process.execSync(
						`./loser ${test.log.alias} ${test.log.number} data/${test.directory} > your_answer/${test.directory}/${subcommand}`,
						{ timeout: TIME_LIMIT });
					break;
				case "commit":
					child_process.execSync(
						`./loser ${test.commit.alias} data/${test.directory}`,
						{ timeout: TIME_LIMIT });
					break;
			}
		} catch (error) {
			if (error.message == "spawnSync /bin/sh ETIMEDOUT") {
				console.log("超時".red);
				continue;
			}
		}

		// 處理 commit 的特殊狀況，判斷掉不存在 .loser_record 的情形
		// 若應存在則複製一份到 your_answer 之中
		if (subcommand == "commit") {
			if (test.commit.not_exist && !fs.existsSync(`data/${test.directory}/.loser_record`)) {
				passed += 1;
				console.log("通過。.loser_record 不存在".green);
				continue;
			} else if (test.commit.not_exist && fs.existsSync(`data/${test.directory}/.loser_record`)) {
				console.log("不該建立 .loser_record".red);
				continue;
			}
			try {
				child_process.execSync(`cp data/${test.directory}/.loser_record your_answer/${test.directory}/commit`)
			} catch (error) {
				console.log(`複製 data/${test.directory}/.loser_record 到 your_answer/${test.directory}/commit 失敗`.red);
				continue;
			}
		}

		const standard_answer_path = `your_answer/${test.directory}/${subcommand}`;
		const your_answer_path = ` standard_answer/${test.directory}/${subcommand}`;
		try {
			// diff 結果若不相同狀態碼不爲 0 ，會觸發例外
			const result = child_process.execSync(`diff ${standard_answer_path} ${your_answer_path}`);
		} catch (error) {
			console.log(`錯誤。請比對 ${standard_answer_path} 與 ${your_answer_path}`.red);
			continue;
		}
		// 並無觸發任何例外，通過
		passed += 1;
		console.log("通過".green);
	}
	console.log("");
	console.log(`${subcommand} 子指令的測試共通過 ${passed}/${total}`);
	if (total == passed) { console.log(`恭喜！通過 ${subcommand} 子指令的所有測資`); }
	else { console.log("請再接再厲！強者的道路不是筆直的"); }
	console.log("");

	return (total == passed);
}

function judge_status(test_suite) {
	return judge_stdout(test_suite, "status");
}

function judge_log(test_suite) {
	return judge_stdout(test_suite, "log");
}

function judge_commit(test_suite) {
	return judge_stdout(test_suite, "commit");
}

const subcommand_test_script = [
	"./1_empty.js",
	"./2_only_a.js",
	"./3_commit_2.js",
	"./4_one_large_file.js",
	"./5_many_files.js",
	"./6_large_record.js"
];

const subcommand_test_suite = subcommand_test_script.map((s) => require(s));

const alias_test_script = [ "./7_alias.js" ];

const alias_test_suite = alias_test_script.map((s) => require(s));

function to_point(passed) {
	return passed ? 2 : 0;
}

function judge_alias(alias_test_suite) {
	const status_passed = judge_status(alias_test_suite);
	const log_passed = judge_log(alias_test_suite);
	// 順序很重要，會破壞資料的 commit 必須放在最末
	const commit_passed = judge_commit(alias_test_suite);
	return status_passed && log_passed && commit_passed;
}

function judge_all(subcommand_test_suite, alias_test_suite) {
	const status_passed = judge_status(subcommand_test_suite);
	const log_passed = judge_log(subcommand_test_suite);
	// 順序很重要，會破壞資料的 commit 必須放在最末
	const commit_passed = judge_commit(subcommand_test_suite);
	function report(name, passed) {
		if (passed) {
			console.log(`${name} 通過。`.green);
		} else {
			console.log(`${name} 失敗。`.red);
		}
	}
	report("status", status_passed);
	report("log", log_passed);
	report("commit", commit_passed);

	let point = to_point(log_passed) + to_point(commit_passed) + to_point(status_passed);
	if (point != 6) {
		console.log(`共取得 ${point} / 8 分`)
		console.log("子指令未完全正確，不繼續測試別名功能".red);
	} else {
		console.log("\n################################################################\n");
		console.log("子指令完全正確，開始測試別名");
		point += to_point(judge_alias(alias_test_suite));
		if (point == 8) {
			console.log("別名測試通過".green);
			console.log(`共取得 ${point} / 8 分`)
		} else {
			console.log("別名測試失敗".red);
			console.log(`共取得 ${point} / 8 分`);
		}
	}
}


if (argv._.length == 0) {
	create_data(subcommand_test_suite);
	create_answer(subcommand_test_suite);
	create_data(alias_test_suite);
	create_answer(alias_test_suite);
	judge_all(subcommand_test_suite, alias_test_suite);
} else {
	switch (argv._[0]) {
		case "create":
			create_data(subcommand_test_suite);
			create_answer(subcommand_test_suite);
			create_data(alias_test_suite);
			create_answer(alias_test_suite);
			break;
		case "all":
			create_data(subcommand_test_suite);
			create_answer(subcommand_test_suite);
			create_data(alias_test_suite);
			create_answer(alias_test_suite);
			judge_all(subcommand_test_suite, alias_test_suite);
			break;
		case "status":
			create_data(subcommand_test_suite);
			create_answer(subcommand_test_suite);
			judge_status(subcommand_test_suite);
			break;
		case "commit":
			create_data(subcommand_test_suite);
			create_answer(subcommand_test_suite);
			judge_commit(subcommand_test_suite);
			break;
		case "log":
			create_data(subcommand_test_suite);
			create_answer(subcommand_test_suite);
			judge_log(subcommand_test_suite);
			break;
		case "alias":
			create_data(alias_test_suite);
			create_answer(alias_test_suite);
			judge_alias(alias_test_suite);
			break;
		case "clean":
			child_process.execSync("rm -rf data your_answer standard_answer");
			break
	}
}
