module.exports = {
	LARGE_RECORD_SIZE: 2 * 1024 * 1024,
	// 註解上一行，並打開以下註解以將測資六的 .loser_record 檔案調到極限大小
	// 這裡並非 2 GB 的原因是腳本所建立的檔案會略微超過我們在此設定的 LARGE_RECORD_SIZE
	// 若設爲 2 GB 就超過了
	// 注意：會花上數十秒來建立此檔案
	// LARGE_RECORD_SIZE: 1024 * 1024 * 1024,
}
