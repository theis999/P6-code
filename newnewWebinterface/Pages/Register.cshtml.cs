using System.Security.Claims;
using System.Text.Json.Nodes;
using Microsoft.AspNetCore.Authorization;
using Microsoft.AspNetCore.Mvc;
using Microsoft.AspNetCore.Mvc.RazorPages;

namespace newnewWebinterface.Pages;

[Authorize]
public class RegisterModel : PageModel
{
    [BindProperty]
    public string name { get; set; }

    [BindProperty]
    public string ProductID { get; set; }

    private readonly HttpClient client;

    public RegisterModel(IHttpClientFactory httpClientFactory)
    {
        client = httpClientFactory.CreateClient();
    }

    public async Task<IActionResult> OnPostAsync()
    {
        var authentikUserID = User.FindFirst(ClaimTypes.NameIdentifier)?.Value;

        var content = new JsonObject
        {
            ["name"] = name,
            ["ProductID"] = ProductID,
            ["authentikUserID"] = authentikUserID
        };

        await client.PostAsJsonAsync("https://smakdb.head9x.dk/boards", content);

        return Page();
    }
}